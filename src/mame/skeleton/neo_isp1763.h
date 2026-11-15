// license:BSD-3-Clause
// copyright-holders:SamDelorean contributors
// ISP1763A register and SRAM backplane for the NEO 2013 USB HC/DC firmware.
// Philips/ST ISP1763A datasheet Rev04. No peripheral, packet, or IRQ is
// synthesized without a linked device. This is a hardware core, not a claim
// of working USB host/printer or host-side PC enumeration.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace neo {
class isp1763 {
public:
    using u8=std::uint8_t;
    using u16=std::uint16_t;
    using u32=std::uint32_t;
    static constexpr u32 chip_id=0x00176320U;
    static constexpr std::size_t host_ram_size=24*1024;
    static constexpr std::size_t device_ram_size=4*1024;
    isp1763(){reset();}
    void reset() {
        m_regs.fill(0);m_host_ram.fill(0);m_dc_ram.fill(0);
        m_ep={};for(std::size_t i=0;i<m_ep.size();++i){m_ep[i].base=u16(i*64);m_ep[i].capacity=64;}
        m_hc_addr=0;m_ep_select=0x20;m_dma_ep=0;
        m_host_irq=m_host_enable=0;m_device_irq=m_device_enable=0;
        m_unlocked=true;m_regs[0x80/2]=0x001e;
        m_regs[0xD4/2]=0x0040; // HC CLK_READY after reset
    }
    // ISP1763 CPU bus is 16-bit; 32-bit registers are LOW word then HIGH word.
    // Firmware explicitly exchanges word halves when constructing a BE 68k dword.
    u16 read(u16 offset) {
        if(offset&1||offset>=0x100)return 0xffff;
        switch(offset){
        case 0x70:return u16(chip_id);
        case 0x72:return u16(chip_id>>16);
        case 0xC4:return m_hc_addr;
        case 0xC6:return host_transfer(0,false);
        case 0xD4:return m_host_irq|0x40;
        case 0xD6:return m_host_enable;
        case 0x14:return u16(m_device_enable);
        case 0x16:return u16(m_device_enable>>16);
        case 0x18:return u16(m_device_irq);
        case 0x1a:return u16(m_device_irq>>16);
        case 0x20:return dc_transfer(0,false);
        case 0x1c:return m_ep[ep_index()].length;
        case 0x1e:return m_ep[ep_index()].ready?1:0;
        case 0x2c:return m_ep_select;
        case 0x58:return m_dma_ep;
        case 0xb8:return 0; // SW_RESET bits self-clear
        default:return known(offset)?m_regs[offset/2]:0xffff;
        }
    }
    void write(u16 offset,u16 data,u16 mask=0xffff){
        if(offset&1||offset>=0x100)return;
        auto merge=[&](u16 &v){v=u16((v&~mask)|(data&mask));};
        switch(offset){
        case 0x70:case 0x72:return; // read-only silicon ID
        case 0x7c:if((data&mask)==0xaa37)m_unlocked=true;return;
        case 0xb8:
            if(data&1){reset();return;}
            if(data&2){reset_host();return;}
            // Reset ATX affects physical transceiver, not guest-owned files.
            return;
        case 0xc4:merge(m_hc_addr);return;
        case 0xc6:host_transfer(data,true);return;
        case 0xd4:m_host_irq &=~(data&mask);return; // W1C
        case 0xd6:merge(m_host_enable);return;
        case 0x14:write32(m_device_enable,data,mask,false);return;
        case 0x16:write32(m_device_enable,data,mask,true);return;
        case 0x18:clear32(m_device_irq,data,mask,false);return;
        case 0x1a:clear32(m_device_irq,data,mask,true);return;
        case 0x2c:m_ep_select=u8(data&0x2f);m_ep[ep_index()].cursor=0;return;
        case 0x58:m_dma_ep=u8(data&0x0f);return;
        case 0x1c:merge(m_ep[ep_index()].length);return;
        case 0x20:dc_transfer(data,true);return;
        case 0x28: {
            u16 const v=data&mask;
            auto &ep=m_ep[ep_index()];
            if(v&0x10){ep.length=0;ep.cursor=0;ep.ready=false;}
            if(v&0x08)ep.ready=true; // VENDP validates IN bank
            if(v&0x01)ep.halted=true;
            m_regs[offset/2]=v&0x1f;return;
        }
        case 0x04: {
            auto &ep=m_ep[ep_index()];
            u16 const size=data&0x7ff;
            if(size && size<=1024)resize_endpoint(ep_index(),size);
            m_regs[offset/2]=data;return;
        }
        default:
            if(known(offset) && m_unlocked)merge(m_regs[offset/2]);
            return;
        }
    }
    // External transport adapters call these only after real USB packets.
    // In their absence endpoints remain disconnected; no fabricated ACKs.
    bool inject_out(u8 endpoint,const u8 *bytes,u16 count) {
        if(endpoint>=m_ep.size()||!bytes)return false;
        auto &ep=m_ep[endpoint];
        if(ep.halted||!ep.capacity||count>ep.capacity)return false;
        if(ep.base+count>m_dc_ram.size())return false;
        std::copy_n(bytes,count,m_dc_ram.begin()+ep.base);
        ep.length=count;ep.cursor=0;ep.ready=true;
        m_device_irq |= endpoint==16?u32(1)<<8:u32(1)<<(10+endpoint);
        return true;
    }
    bool take_in(u8 endpoint,u8 *bytes,u16 &count) {
        if(endpoint>=m_ep.size()||!bytes)return false;
        auto &ep=m_ep[endpoint];
        if(!ep.ready||ep.halted||ep.length>count)return false;
        std::copy_n(m_dc_ram.begin()+ep.base,ep.length,bytes);
        count=ep.length;ep.ready=false;ep.length=0;ep.cursor=0;
        m_device_irq |= endpoint==16?u32(1)<<8:u32(1)<<(10+endpoint);
        return true;
    }
    bool irq() const {return bool((m_device_irq&m_device_enable) || (m_host_irq&m_host_enable));}
    u16 host_memory_address() const {return m_hc_addr;}
    std::size_t host_size() const {return m_host_ram.size();}
private:
    struct endpoint_state { u16 base=0,capacity=64,length=0,cursor=0;bool ready=false,halted=false; };
    std::array<u16,0x80>m_regs{};
    std::array<u8,host_ram_size>m_host_ram{};
    std::array<u8,device_ram_size>m_dc_ram{};
    std::array<endpoint_state,17>m_ep{};
    u16 m_hc_addr=0,m_host_irq=0,m_host_enable=0;
    u32 m_device_irq=0,m_device_enable=0;
    u8 m_ep_select=0x20,m_dma_ep=0;
    bool m_unlocked=true;
    std::size_t ep_index() const {return (m_ep_select&0x20)?16:std::size_t(m_ep_select&0x0f);}
    bool resize_endpoint(std::size_t which,u16 size){
        std::size_t total=0;
        for(std::size_t i=0;i<m_ep.size();++i)total+=(i==which)?size:m_ep[i].capacity;
        if(total>device_ram_size)return false;
        std::array<u8,device_ram_size> next{};
        std::size_t base=0;
        for(std::size_t i=0;i<m_ep.size();++i){
            auto &e=m_ep[i];
            const u16 new_capacity=i==which?size:e.capacity;
            const u16 copy=u16(std::min<u16>(e.length,new_capacity));
            if(copy)std::copy_n(m_dc_ram.begin()+e.base,copy,next.begin()+base);
            e.base=u16(base);e.capacity=new_capacity;e.length=copy;
            e.cursor=std::min<u16>(e.cursor,copy);
            base+=new_capacity;
        }
        m_dc_ram=next;return true;
    }
    void reset_host(){
        m_host_ram.fill(0);m_hc_addr=m_host_irq=m_host_enable=0;
        m_regs[0xd4/2]=0x40;
    }
    static void write32(u32 &v,u16 data,u16 mask,bool high) {
        const u32 shift=high?16:0;
        const u32 m=u32(mask)<<shift;
        v=(v&~m)|((u32(data&mask))<<shift);
    }
    static void clear32(u32 &v,u16 data,u16 mask,bool high) {
        v &= ~(u32(data&mask)<<(high?16:0));
    }
    u16 host_transfer(u16 value,bool wr){
        if(m_hc_addr>=m_host_ram.size()-1)return 0xffff;
        const std::size_t p=m_hc_addr;
        u16 result=u16(m_host_ram[p])|(u16(m_host_ram[p+1])<<8);
        if(wr){m_host_ram[p]=u8(value);m_host_ram[p+1]=u8(value>>8);}
        m_hc_addr=u16(m_hc_addr+2);
        return result;
    }
    u16 dc_transfer(u16 value,bool wr){
        auto &ep=m_ep[ep_index()];
        if(ep.halted||ep.cursor>=ep.capacity||ep.base+ep.cursor>=m_dc_ram.size())return 0xffff;
        const std::size_t p=ep.base+ep.cursor;
        if(p+1>=m_dc_ram.size())return 0xffff;
        const u16 result=u16(m_dc_ram[p])|(u16(m_dc_ram[p+1])<<8);
        if(wr){m_dc_ram[p]=u8(value);m_dc_ram[p+1]=u8(value>>8);ep.length=std::max<u16>(ep.length,u16(ep.cursor+2));}
        ep.cursor=u16(ep.cursor+2);
        return result;
    }
    static bool known(u16 offset){
        switch(offset){
            case 0x00:case 0x04:case 0x08:case 0x0c:case 0x10:
            case 0x24:case 0x28:case 0x2c:case 0x30:case 0x34:case 0x38:
            case 0x58:case 0x74:case 0x78:case 0x7c:case 0x80:
            case 0x88:case 0x8c:case 0x90:case 0x94:case 0x98:
            case 0x9c:case 0xa0:case 0xa2:case 0xa4:case 0xa6:
            case 0xb0:case 0xb2:case 0xb4:case 0xb6:case 0xba:
            case 0xbc:case 0xc0:case 0xc2:case 0xc8:case 0xca:
            case 0xcc:case 0xce:case 0xd0:case 0xd2:case 0xd8:
            case 0xda:case 0xdc:case 0xde:case 0xe0:case 0xe2:
                return true;
            default:return false;
        }
    }
};
} // namespace neo
