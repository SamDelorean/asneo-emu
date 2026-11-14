// license:BSD-3-Clause
// copyright-holders:SamDelorean contributors
// AlphaSmart NEO: MAME hardware-boot scaffold, NOT a qualified emulator.
// CPU address-width technique reuses SamDelorean/DANA-emu; segmented firmware,
// keyboard and LCD protocols are cross-checked against ioma8/neo-re.
//
// Important: VZ PLL/RTC/IRQ/NOR CFI/USB have NOT been transplanted yet.
// Unsupported operations are logged; this driver is MACHINE_NOT_WORKING.
// No invented boot screens, no OS trap shims, no Dana-specific peripherals.
#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "screen.h"
#include "emupal.h"

#include "neo_firmware.h"
#include "neo_keyboard.h"
#include "neo_lcd.h"

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
        :driver_device(cfg,type,tag),m_cpu(*this,"maincpu"),m_rows(*this,"ROW%u",0U) {}
    void asneo(machine_config &cfg);
protected:
    virtual void machine_start() override ATTR_COLD;
    virtual void machine_reset() override ATTR_COLD;
private:
    required_device<neo_vz_cpu_device> m_cpu;
    required_ioport_array<16> m_rows;
    std::vector<u8> m_sram, m_flash;
    neo::keyboard m_keyboard;
    neo::lcd m_lcd;
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
    m_flash.assign(0x200000,0xff);
    m_registers.fill(0);
    const auto *small=memregion("smallrom");
    const auto *system=memregion("system");
    std::string error;
    m_firmware_valid=neo::firmware::install_small(small->base(),small->bytes(),m_flash,error);
    if(m_firmware_valid)
        m_firmware_valid=neo::firmware::install_system(system->base(),system->bytes(),m_flash,error);
    if(!m_firmware_valid) {
        logerror("NEO_FIRMWARE_IMPORT_FAIL %s\n",error.c_str());
        // No fake reset vector or fabricated successful OS boot.
        m_cpu->suspend(SUSPEND_REASON_HALT,true);
    } else {
        logerror("NEO_FIRMWARE_IMPORT_OK (segment assembly only, not boot PASS)\n");
    }
}
void alphasneo_state::machine_reset() {
    if(!m_firmware_valid)return;
    m_keyboard.reset();m_lcd.reset();m_registers.fill(0);
    // SmallROM mirror is needed for the actual 68k reset vector fetch.
    std::copy_n(m_flash.begin(),0x100,m_sram.begin());
}
u8 alphasneo_state::vz_read8(u16 r) {
    if(r==0xf419) {
        for(unsigned i=0;i<16;++i)m_keyboard.set_row(i,m_rows[i]->read());
        return m_keyboard.columns();
    }
    // Only verified board observation: bit 5 of F449 is set in neo-re.
    if(r==0xf449)return m_registers[r&0xfff]|0x20;
    if(r==0xf410 || r==0xf408 || r==0xf440)return m_registers[r&0xfff];
    logerror("NEO_VZ_UNIMPLEMENTED_READ %04x\n",r);
    return 0xff;
}
void alphasneo_state::vz_write8(u16 r,u8 value) {
    if(r==0xf410 || r==0xf408 || r==0xf440) {
        m_registers[r&0xfff]=value;
        m_keyboard.gpio_write(r,value);
        return;
    }
    logerror("NEO_VZ_UNIMPLEMENTED_WRITE %04x=%02x\n",r,value);
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
    u32 a=offset<<1;
    return (u16(m_flash[a])<<8)|m_flash[a+1];
}
void alphasneo_state::flash_w(offs_t offset,u16 data,u16 mask) {
    logerror("NEO_FLASH_CFI_UNIMPLEMENTED at %08x value=%04x mask=%04x\n",0x00400000+(offset<<1),data,mask);
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
    map(0x00400000,0x005fffff).rw(FUNC(alphasneo_state::flash_r),FUNC(alphasneo_state::flash_w));
    map(0x01000000,0x01000001).rw(FUNC(alphasneo_state::lcd_right_r),FUNC(alphasneo_state::lcd_right_w));
    map(0x01008000,0x01008001).rw(FUNC(alphasneo_state::lcd_left_r),FUNC(alphasneo_state::lcd_left_w));
    map(0xfffff000,0xffffffff).rw(FUNC(alphasneo_state::vz_r),FUNC(alphasneo_state::vz_w));
    // ASIC 0x02000000 and VZ IRQ/RTC are not implemented; no fabricated data.
}
static INPUT_PORTS_START( asneo )
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
    screen_device &screen(SCREEN(cfg,"screen",SCREEN_TYPE_LCD));
    screen.set_refresh_hz(60);
    screen.set_size(neo::lcd::width,neo::lcd::height);
    screen.set_visarea_full();
    screen.set_screen_update(FUNC(alphasneo_state::screen_update));
    screen.set_palette("palette");
    PALETTE(cfg,"palette",FUNC(alphasneo_state::palette_init),2);
}
ROM_START(asneo)
    // Files are user-provided update packages, not a complete physical NOR dump.
    // No hash here: private ROMs are intentionally not part of this public repo.
    ROM_REGION(0x581a,"smallrom",0)
    ROM_LOAD("smallos3kneorom.os3kos",0,0x581a,"")
    ROM_REGION(0x60800,"system",0)
    ROM_LOAD("os3kneorom.os3kos",0,0x60800,"")
ROM_END
} // anonymous namespace
COMP(2004,asneo,0,0,asneo,asneo,alphasneo_state,empty_init,
     "AlphaSmart","AlphaSmart NEO (hardware-boot scaffold)",
     MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW)
