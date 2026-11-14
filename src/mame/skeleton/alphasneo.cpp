// license:BSD-3-Clause
// copyright-holders:SamDelorean contributors
// AlphaSmart NEO: MAME hardware-boot scaffold, NOT a qualified emulator.
// CPU address-width technique reuses SamDelorean/DANA-emu; segmented firmware,
// keyboard and LCD protocols are cross-checked against ioma8/neo-re.
//
// VZ PLL/RTC/Timer1/2/IRQ core is adapted from Dana. NOR CFI, USB and physical
// low-power timing remain pending qualification with NEO firmware.
// ST M29W320EB NOR profile is provisional, never an ID of the user’s board.
// Unsupported operations are logged; this driver is MACHINE_NOT_WORKING.
// No invented boot screens, no OS trap shims, no Dana-specific peripherals.
#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "screen.h"
#include "emupal.h"

#include "neo_firmware.h"
#include "neo_keyboard.h"
#include "neo_lcd.h"
#include "neo_vz_core.h"
#include "neo_nor.h"
#include "machine/nvram.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

// Directly adapted from the existing DANA-emu VZ-aware M68000 carrier.
// Stock MC68EZ328 is not a substitute for NEO's MC68VZ328.
class neo_vz_cpu_device;
DECLARE_DEVICE_TYPE(NEO_VZ_CPU, neo_vz_cpu_device)
class neo_vz_cpu_device : public m68000_device {
public:
    neo_vz_cpu_device(const machine_config &cfg,const char *tag,device_t *owner,u32 clock)
        : m68000_device(cfg,NEO_VZ_CPU,tag,owner,clock) {
        m_program_config.m_addr_width=32;
        m_opcodes_config.m_addr_width=32;
        m_uprogram_config.m_addr_width=32;
        m_uopcodes_config.m_addr_width=32;
        m_cpu_space_config.m_addr_width=32;
    }
};
DEFINE_DEVICE_TYPE(NEO_VZ_CPU,neo_vz_cpu_device,"neo_vz_cpu","AlphaSmart NEO VZ bus")

namespace {
class alphasneo_state : public driver_device {
public:
    alphasneo_state(const machine_config &cfg,device_type type,const char *tag)
        :driver_device(cfg,type,tag),m_cpu(*this,"maincpu"),
         m_rows(*this,"ROW%u",0U),m_nor_config(*this,"NOR_PROFILE"),
         m_flash_nvram(*this,"flashnv"),m_ram_nvram(*this,"ramnv") {}
    void asneo(machine_config &cfg);
protected:
    virtual void machine_start() override ATTR_COLD;
    virtual void machine_reset() override ATTR_COLD;
private:
    required_device<neo_vz_cpu_device> m_cpu;
    required_ioport_array<16> m_rows;
    required_ioport m_nor_config;
    required_device<nvram_device> m_flash_nvram;
    required_device<nvram_device> m_ram_nvram;
    std::vector<u8> m_sram, m_flash, m_flash_seed;
    neo::nor_flash m_nor;
    void nvram_seed_flash(nvram_device &device,void *base,size_t bytes);
    neo::keyboard m_keyboard;
    neo::lcd m_lcd;
    neo::vz_core m_vz;
    emu_timer *m_clk32_timer=nullptr;
    std::array<emu_timer *,2> m_compare_timers{};
    void update_vz_irq();
    void update_vz_clock();
    u16 timer_count(unsigned n) const;
    void timer_schedule(unsigned n);
    TIMER_CALLBACK_MEMBER(clk32_tick);
    TIMER_CALLBACK_MEMBER(timer1_compare);
    TIMER_CALLBACK_MEMBER(timer2_compare);
    void cpu_space_map(address_map &map) ATTR_COLD;
    u8 irq_vector_r(offs_t offset);
    std::array<u8,4096> m_registers{}; // currently known GPIO latch values only
    bool m_firmware_valid=false;
    void mem_map(address_map &map) ATTR_COLD;
    u16 ram_r(offs_t offset,u16 mem_mask=0xffff);
    void ram_w(offs_t offset,u16 data,u16 mem_mask=0xffff);
    u16 flash_r(offs_t offset,u16 mem_mask=0xffff);
    void flash_w(offs_t offset,u16 data,u16 mem_mask=0xffff);
    u16 vz_r(offs_t offset,u16 mem_mask=0xffff);
    void vz_w(offs_t offset,u16 data,u16 mem_mask=0xffff);
    u8 vz_read8(u16 reg);
    void vz_write8(u16 reg,u8 value);
    u16 lcd_left_r(offs_t offset,u16 mem_mask=0xffff) {return lcd_read(0,mem_mask);}
    u16 lcd_right_r(offs_t offset,u16 mem_mask=0xffff) {return lcd_read(1,mem_mask);}
    void lcd_left_w(offs_t offset,u16 value,u16 mem_mask=0xffff) {lcd_write(0,value,mem_mask);}
    void lcd_right_w(offs_t offset,u16 value,u16 mem_mask=0xffff) {lcd_write(1,value,mem_mask);}
    u16 lcd_read(unsigned n,u16 mem_mask);
    void lcd_write(unsigned n,u16 value,u16 mem_mask);
    u32 screen_update(screen_device &,bitmap_ind16 &bitmap,const rectangle &cliprect);
    void palette_init(palette_device &palette) const;
};
void alphasneo_state::machine_start() {
    m_sram.assign(0x80000,0);
    m_flash.assign(neo::nor_flash::size_bytes,0xff);
    m_flash_seed.assign(neo::nor_flash::size_bytes,0xff);
    // NVRAM owns virtual media only. Original update packages are untouched.
    m_flash_nvram->set_base(m_flash.data(),m_flash.size());
    m_ram_nvram->set_base(m_sram.data(),m_sram.size());
    m_nor.attach(m_flash.data(),m_flash.size());
    m_registers.fill(0);
    m_clk32_timer=timer_alloc(FUNC(alphasneo_state::clk32_tick),this);
    m_compare_timers[0]=timer_alloc(FUNC(alphasneo_state::timer1_compare),this);
    m_compare_timers[1]=timer_alloc(FUNC(alphasneo_state::timer2_compare),this);
    std::string error;
    // Single NEO driver, interchangeable private ROM sets. The original
    // asneo set still imports the two authentic update packages. BOOT/WRITER
    // sets load an assembled 4 MiB virtual NOR image, never a physical dump.
    if (auto *image=memregion("flashrom")) {
        m_firmware_valid=(image->bytes()==m_flash_seed.size() &&
            neo::firmware::be32(image->base())==0x0007fff0 &&
            neo::firmware::be32(image->base()+4)==0x0040042a);
        if(m_firmware_valid)
            std::copy_n(image->base(),image->bytes(),m_flash_seed.begin());
        else error="Invalid NEO full-flash test image size or reset vectors";
    } else {
        auto *small=memregion("smallrom");
        auto *system=memregion("system");
        m_firmware_valid=(small && system &&
            neo::firmware::install_small(small->base(),small->bytes(),m_flash_seed,error));
        if(m_firmware_valid)
            m_firmware_valid=neo::firmware::install_system(system->base(),system->bytes(),m_flash_seed,error);
    }
    if(!m_firmware_valid) {
        logerror("NEO_FIRMWARE_IMPORT_FAIL %s\n",error.c_str());
        // No fake reset vector or fabricated successful OS boot.
        m_cpu->suspend(SUSPEND_REASON_HALT,true);
    } else {
        logerror("NEO_FIRMWARE_IMPORT_OK (segment assembly only, not boot PASS)\n");
    }
}
void alphasneo_state::nvram_seed_flash(nvram_device &,void *base,size_t size) {
    // Called only if MAME has no saved Flash image. Do not reflash on reset.
    if(m_firmware_valid && size==m_flash_seed.size())
        std::copy(m_flash_seed.begin(),m_flash_seed.end(),static_cast<u8 *>(base));
    else std::fill_n(static_cast<u8 *>(base),size,0xff);
    logerror("NEO_FLASH_FACTORY_SEED virtual_data=1 valid=%u size=%zu\n",
        unsigned(m_firmware_valid),size);
}
void alphasneo_state::machine_reset() {
    if(!m_firmware_valid)return;
    m_keyboard.reset();m_lcd.reset();m_registers.fill(0);
    m_nor.select_profile((m_nor_config->read()&1)?
        neo::nor_flash::profile::m29w320eb:
        neo::nor_flash::profile::unknown_readonly);
    m_vz.reset();
    system_time host;machine().current_datetime(host);
    const auto &now=host.local_time;
    m_vz.seed_rtc(now.hour,now.minute,now.second);
    m_cpu->set_unscaled_clock(m_vz.sysclk_hz());
    for(auto &t:m_compare_timers)t->adjust(attotime::never);
    m_clk32_timer->adjust(attotime::from_hz(neo::vz_core::xtal_hz),0,
        attotime::from_hz(neo::vz_core::xtal_hz));
    update_vz_irq();
    // SmallROM mirror is needed for the actual 68k reset vector fetch.
    std::copy_n(m_flash.begin(),0x100,m_sram.begin());
}
// MAME virtual timing and IRQ inputs: extracted from the qualified Dana VZ
// implementation. All unmodeled board pins remain disconnected.
void alphasneo_state::update_vz_irq() {
    for(unsigned level=1;level<=6;++level)
        m_cpu->set_input_line(level,m_vz.irq_at_level(level)?ASSERT_LINE:CLEAR_LINE);
}
void alphasneo_state::update_vz_clock() {
    const u32 hz=m_vz.sysclk_hz();
    if(hz)m_cpu->set_unscaled_clock(hz);
    for(unsigned n=0;n<2;++n)timer_schedule(n);
}
u16 alphasneo_state::timer_count(unsigned n) const {
    auto const &t=m_vz.timer_state(n);
    u32 hz=m_vz.timer_source_hz(n);
    if(!(t.ctl&1)||!hz||!m_compare_timers[n]->enabled())return t.cnt;
    u64 delta=m_compare_timers[n]->elapsed().as_ticks(hz)/u64(t.prer+1);
    return u16((u64(t.cnt)+delta)&0xffff);
}
void alphasneo_state::timer_schedule(unsigned n) {
    const auto &t=m_vz.timer_state(n);
    u32 hz=m_vz.timer_source_hz(n);
    if(!(t.ctl&1)||!hz||!t.cmp) {
        m_compare_timers[n]->adjust(attotime::never);
        return;
    }
    u32 remaining=(u32(t.cmp)-u32(t.cnt))&0xffff;
    if(!remaining)remaining=65536;
    m_compare_timers[n]->adjust(attotime::from_ticks(u64(t.prer+1)*remaining,hz));
}
TIMER_CALLBACK_MEMBER(alphasneo_state::timer1_compare) {
    m_vz.compare(0);update_vz_irq();timer_schedule(0);
}
TIMER_CALLBACK_MEMBER(alphasneo_state::timer2_compare) {
    m_vz.compare(1);update_vz_irq();timer_schedule(1);
}
TIMER_CALLBACK_MEMBER(alphasneo_state::clk32_tick) {
    m_vz.advance_clk32();
    if(m_vz.watchdog_trip()) {
        logerror("NEO_VZ_WATCHDOG_RESET_REQUEST (reset action not yet hardware-qualified)\n");
        m_vz.clear_watchdog_trip();
    }
    update_vz_irq();
}
u8 alphasneo_state::irq_vector_r(offs_t offset) {
    return m_vz.irq_vector(offset&7);
}
void alphasneo_state::cpu_space_map(address_map &map) {
    map(0xfffffff0,0xffffffff).r(FUNC(alphasneo_state::irq_vector_r)).umask16(0x00ff);
}
u8 alphasneo_state::vz_read8(u16 r) {
    if(r==0xf419) {
        for(unsigned i=0;i<16;++i)m_keyboard.set_row(i,m_rows[i]->read());
        return m_keyboard.columns();
    }
    if(r==0xf449)return m_registers[r&0xfff]|0x20;
    if(r==0xf410 || r==0xf408 || r==0xf440)return m_registers[r&0xfff];
    if(r==0xf608||r==0xf609) {
        auto value=timer_count(0);
        return r&1?u8(value):u8(value>>8);
    }
    if(r==0xf618||r==0xf619) {
        auto value=timer_count(1);
        return r&1?u8(value):u8(value>>8);
    }
    if(auto value=m_vz.read_word(r&u16(0xfffe)))
        return (r&1)?u8(*value):u8(*value>>8);
    logerror("NEO_VZ_UNIMPLEMENTED_READ %04x\n",r);
    return 0xff;
}
void alphasneo_state::vz_write8(u16 r,u8 value) {
    if(r==0xf410 || r==0xf408 || r==0xf440) {
        m_registers[r&0xfff]=value;
        m_keyboard.gpio_write(r,value);
        return;
    }
    const u16 aligned=r&u16(0xfffe);
    if((aligned>=0xf600 && aligned<=0xf61a) &&
       ((aligned>=0xf600 && aligned<=0xf60a)||(aligned>=0xf610 && aligned<=0xf61a))) {
        unsigned n=aligned<0xf610?0:1;
        if(m_vz.timer_state(n).ctl&1)m_vz.timer_state(n).cnt=timer_count(n);
    }
    const u32 before=m_vz.sysclk_hz();
    const u16 mask=(r&1)?0x00ff:0xff00;
    const u16 data=(r&1)?value:u16(value)<<8;
    if(!m_vz.write_word(aligned,data,mask)) {
        logerror("NEO_VZ_UNIMPLEMENTED_WRITE %04x=%02x\n",r,value);
        return;
    }
    if(before!=m_vz.sysclk_hz())update_vz_clock();
    else if((aligned>=0xf600&&aligned<=0xf60a)||
            (aligned>=0xf610&&aligned<=0xf61a))
        timer_schedule(aligned<0xf610?0:1);
    update_vz_irq();
}
u16 alphasneo_state::vz_r(offs_t offset,u16 mask) {
    u16 r=0xf000+(offset<<1);
    u16 result=0xffff;
    if(mask&0xff00)result=(result&0x00ff)|(u16(vz_read8(r))<<8);
    if(mask&0x00ff)result=(result&0xff00)|vz_read8(r+1);
    return result;
}
void alphasneo_state::vz_w(offs_t offset,u16 data,u16 mask) {
    u16 r=0xf000+(offset<<1);
    if(mask&0xff00)vz_write8(r,data>>8);
    if(mask&0x00ff)vz_write8(r+1,data&0xff);
}
u16 alphasneo_state::ram_r(offs_t offset,u16 mask) {
    u32 a=offset<<1;
    if(a>=0x0000f000 && a<0x00010000)return vz_r((a-0xf000)>>1,mask);
    return (u16(m_sram[a])<<8)|m_sram[a+1];
}
void alphasneo_state::ram_w(offs_t offset,u16 data,u16 mask) {
    u32 a=offset<<1;
    if(a>=0x0000f000 && a<0x00010000) {vz_w((a-0xf000)>>1,data,mask);return;}
    if(mask&0xff00)m_sram[a]=data>>8;
    if(mask&0x00ff)m_sram[a+1]=data&0xff;
}
u16 alphasneo_state::flash_r(offs_t offset,u16 mask) {
    return m_nor.read_word(u32(offset)<<1);
}
void alphasneo_state::flash_w(offs_t offset,u16 data,u16 mask) {
    const u32 byte_offset=u32(offset)<<1;
    if(!m_nor.write_word(byte_offset,data,mask))
        logerror("NEO_NOR_REJECTED profile=%u addr=%08x value=%04x mask=%04x\n",
            unsigned(m_nor_config->read()&1),0x00400000+byte_offset,data,mask);
}
u16 alphasneo_state::lcd_read(unsigned n,u16 mask) {
    u16 result=0xffff;
    if(mask&0xff00)result=(result&0x00ff)|(u16(m_lcd.status(n))<<8);
    if(mask&0x00ff)result=(result&0xff00)|m_lcd.read(n);
    return result;
}
void alphasneo_state::lcd_write(unsigned n,u16 data,u16 mask) {
    if(mask&0xff00)m_lcd.command(n,data>>8);
    if(mask&0x00ff)m_lcd.write(n,data&0xff);
}
u32 alphasneo_state::screen_update(screen_device &,bitmap_ind16 &bitmap,const rectangle &clip) {
    for(int y=clip.min_y;y<=clip.max_y;++y)
        for(int x=clip.min_x;x<=clip.max_x;++x)
            bitmap.pix(y,x)=m_lcd.pixel(x,y)?1:0;
    return 0;
}
void alphasneo_state::palette_init(palette_device &palette) const {
    palette.set_pen_color(0,rgb_t(192,201,170));
    palette.set_pen_color(1,rgb_t(37,48,42));
}
void alphasneo_state::mem_map(address_map &map) {
    map(0x00000000,0x0007ffff).rw(FUNC(alphasneo_state::ram_r),FUNC(alphasneo_state::ram_w));
    map(0x00400000,0x007fffff).rw(FUNC(alphasneo_state::flash_r),FUNC(alphasneo_state::flash_w));
    map(0x01000000,0x01000001).rw(FUNC(alphasneo_state::lcd_right_r),FUNC(alphasneo_state::lcd_right_w));
    map(0x01008000,0x01008001).rw(FUNC(alphasneo_state::lcd_left_r),FUNC(alphasneo_state::lcd_left_w));
    map(0xfffff000,0xffffffff).rw(FUNC(alphasneo_state::vz_r),FUNC(alphasneo_state::vz_w));
    // ASIC 0x02000000, NOR CFI and external IRQ GPIO are still unmodeled.
}
static INPUT_PORTS_START( asneo )
    PORT_START("NOR_PROFILE")
    PORT_CONFNAME(0x01,0x00,"NEO NOR device candidate")
    PORT_CONFSETTING(0x00,"Unknown device (array read-only)")
    PORT_CONFSETTING(0x01,"ST M29W320EB 4MiB (reported NEO board)")
    PORT_START("ROW0")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Close bracket [0x00]") PORT_CODE(KEYCODE_CLOSEBRACE)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("File 6 [0x10]")
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K [0x20]") PORT_CODE(KEYCODE_K)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I [0x30]") PORT_CODE(KEYCODE_I)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Equals [0x40]") PORT_CODE(KEYCODE_EQUALS)
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 [0x50]") PORT_CODE(KEYCODE_8)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Comma [0x60]") PORT_CODE(KEYCODE_COMMA)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_START("ROW1")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Alt [0x41]") PORT_CODE(KEYCODE_LALT)
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Delete [0x61]") PORT_CODE(KEYCODE_DEL)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_START("ROW2")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("File 7 [0x02]")
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L [0x22]") PORT_CODE(KEYCODE_L)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O [0x32]") PORT_CODE(KEYCODE_O)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("File 8 [0x42]")
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 [0x52]") PORT_CODE(KEYCODE_9)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Period [0x62]") PORT_CODE(KEYCODE_STOP)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_START("ROW3")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Open bracket [0x03]") PORT_CODE(KEYCODE_OPENBRACE)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Quote [0x13]") PORT_CODE(KEYCODE_QUOTE)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Semicolon [0x23]") PORT_CODE(KEYCODE_COLON)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P [0x33]") PORT_CODE(KEYCODE_P)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Minus [0x43]") PORT_CODE(KEYCODE_MINUS)
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 [0x53]") PORT_CODE(KEYCODE_0)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Slash [0x73]") PORT_CODE(KEYCODE_SLASH)
    PORT_START("ROW4")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Command [0x14]") PORT_CODE(KEYCODE_LWIN)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Home [0x34]") PORT_CODE(KEYCODE_HOME)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Row 4 col 5 logical 0x34")
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Esc [0x74]") PORT_CODE(KEYCODE_ESC)
    PORT_START("ROW5")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down [0x15]") PORT_CODE(KEYCODE_DOWN)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("End [0x65]") PORT_CODE(KEYCODE_END)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left [0x75]") PORT_CODE(KEYCODE_LEFT)
    PORT_START("ROW6")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Applets [0x46]")
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right [0x76]") PORT_CODE(KEYCODE_RIGHT)
    PORT_START("ROW7")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Send [0x47]")
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Row 7 col 6 logical 0x3f")
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up [0x77]") PORT_CODE(KEYCODE_UP)
    PORT_START("ROW8")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_START("ROW9")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Backspace [0x09]") PORT_CODE(KEYCODE_BACKSPACE)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("File 5 [0x19]")
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Backslash [0x29]") PORT_CODE(KEYCODE_BACKSLASH)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Print [0x49]")
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Row 9 col 5 logical 0x35")
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter [0x69]") PORT_CODE(KEYCODE_ENTER)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space [0x79]") PORT_CODE(KEYCODE_SPACE)
    PORT_START("ROW10")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("File 3 [0x0a]")
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("File 4 [0x1a]")
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D [0x2a]") PORT_CODE(KEYCODE_D)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E [0x3a]") PORT_CODE(KEYCODE_E)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("File 2 [0x4a]")
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 [0x5a]") PORT_CODE(KEYCODE_3)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C [0x6a]") PORT_CODE(KEYCODE_C)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_START("ROW11")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Caps Lock [0x0b]") PORT_CODE(KEYCODE_CAPSLOCK)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S [0x2b]") PORT_CODE(KEYCODE_S)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W [0x3b]") PORT_CODE(KEYCODE_W)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("File 1 [0x4b]")
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 [0x5b]") PORT_CODE(KEYCODE_2)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X [0x6b]") PORT_CODE(KEYCODE_X)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_START("ROW12")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Tab [0x0c]") PORT_CODE(KEYCODE_TAB)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A [0x2c]") PORT_CODE(KEYCODE_A)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q [0x3c]") PORT_CODE(KEYCODE_Q)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Grave [0x4c]") PORT_CODE(KEYCODE_TILDE)
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 [0x5c]") PORT_CODE(KEYCODE_1)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z [0x6c]") PORT_CODE(KEYCODE_Z)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Control [0x7c]") PORT_CODE(KEYCODE_LCONTROL)
    PORT_START("ROW13")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T [0x0d]") PORT_CODE(KEYCODE_T)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G [0x1d]") PORT_CODE(KEYCODE_G)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F [0x2d]") PORT_CODE(KEYCODE_F)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R [0x3d]") PORT_CODE(KEYCODE_R)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 [0x4d]") PORT_CODE(KEYCODE_5)
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 [0x5d]") PORT_CODE(KEYCODE_4)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V [0x6d]") PORT_CODE(KEYCODE_V)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B [0x7d]") PORT_CODE(KEYCODE_B)
    PORT_START("ROW14")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift [0x0e]") PORT_CODE(KEYCODE_LSHIFT)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Row 14 col 2 logical 0x1a")
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right Shift [0x6e]") PORT_CODE(KEYCODE_RSHIFT)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_START("ROW15")
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y [0x0f]") PORT_CODE(KEYCODE_Y)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H [0x1f]") PORT_CODE(KEYCODE_H)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J [0x2f]") PORT_CODE(KEYCODE_J)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U [0x3f]") PORT_CODE(KEYCODE_U)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 [0x4f]") PORT_CODE(KEYCODE_6)
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 [0x5f]") PORT_CODE(KEYCODE_7)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M [0x6f]") PORT_CODE(KEYCODE_M)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N [0x7f]") PORT_CODE(KEYCODE_N)
INPUT_PORTS_END

void alphasneo_state::asneo(machine_config &cfg) {
    // 33 MHz is the current neo-re timing reference, not physically certified.
    NEO_VZ_CPU(cfg,m_cpu,33'000'000);
    m_cpu->set_addrmap(AS_PROGRAM,&alphasneo_state::mem_map);
    m_cpu->set_addrmap(m68000_base_device::AS_CPU_SPACE,&alphasneo_state::cpu_space_map);
    screen_device &screen(SCREEN(cfg,"screen",SCREEN_TYPE_LCD));
    screen.set_refresh_hz(60);
    screen.set_size(neo::lcd::width,neo::lcd::height);
    screen.set_visarea_full();
    screen.set_screen_update(FUNC(alphasneo_state::screen_update));
    screen.set_palette("palette");
    PALETTE(cfg,"palette",FUNC(alphasneo_state::palette_init),2);
    NVRAM(cfg,"ramnv",nvram_device::DEFAULT_ALL_0);
    NVRAM(cfg,"flashnv",nvram_device::DEFAULT_ALL_1)
        .set_custom_handler(FUNC(alphasneo_state::nvram_seed_flash));
}
ROM_START(asneo)
    // Files are user-provided update packages, not a complete physical NOR dump.
    // Checksums are metadata only; private binaries are never published here.
    ROM_REGION(0x581a,"smallrom",0)
    ROM_LOAD("smallos3kneorom.os3kos",0,0x581a,
        CRC(ed88f65a) SHA1(78a3bcbb81773335f69c8964ff8a114e0d7712cf))
    ROM_REGION(0x60800,"system",0)
    ROM_LOAD("os3kneorom.os3kos",0,0x60800,
        CRC(cbb60113) SHA1(30ebd3be9da12186b8f8146b251f0e21b95e74ad))
ROM_END

// These are private *data* variants, not emulator variants or new hardware.
// The ROMs are constructed outside Git from manufacturer NEO firmware.
ROM_START(asneob)
    ROM_REGION(0x400000,"flashrom",0)
    ROM_LOAD("os3k_neo_min_boot.bin",0,0x400000,
        CRC(0bb36e55) SHA1(5930ee13122d26d690748206407baeec5e33be06))
ROM_END
ROM_START(asneow)
    ROM_REGION(0x400000,"flashrom",0)
    ROM_LOAD("os3k_neo_min_writer.bin",0,0x400000,
        CRC(080b51c3) SHA1(7bf9476ff6eb3384da05cacea8bb20a641043c3a))
ROM_END
} // anonymous namespace
COMP(2004,asneo,0,0,asneo,asneo,alphasneo_state,empty_init,
     "AlphaSmart","AlphaSmart NEO (original update packages)",
     MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW)
COMP(2004,asneob,asneo,0,asneo,asneo,alphasneo_state,empty_init,
     "AlphaSmart","AlphaSmart NEO (minimal BOOT virtual NOR)",
     MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW)
COMP(2004,asneow,asneo,0,asneo,asneo,alphasneo_state,empty_init,
     "AlphaSmart","AlphaSmart NEO (minimal WRITER virtual NOR)",
     MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW)
