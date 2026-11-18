// license:BSD-3-Clause
// ISP1763 HC periodic interrupt PTD / real USB hub port change.
#include "neo_isp1763.h"
#include "neo_usb_function.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
using u8=std::uint8_t; using u16=std::uint16_t; using u32=std::uint32_t;
static void le32(std::array<u8,32>&p,unsigned off,u32 value) {
 for(unsigned i=0;i<4;++i)p[off+i]=u8(value>>(8*i));
}
static void host_write(neo::isp1763 &c,u16 addr,const u8 *data,unsigned n){
 c.write(0xc4,addr);
 for(unsigned i=0;i<n;i+=2)
   c.write(0xc6,u16(data[i])|(u16(i+1<n?data[i+1]:0)<<8));
}
int main(){
 neo::isp1763 c;
 neo_usb::usb_function hub{};
 hub.kind=3;hub.address=2;hub.configuration=1;hub.hub_attach(true);
 c.set_host_attached(true);
 c.write(0xa0,0x1000);
 for(unsigned i=0;i<20;++i)c.tick_1ms();
 c.write(0xa0,0x1100);
 for(unsigned i=0;i<10;++i)c.tick_1ms();
 assert((c.read(0xa0)&0x1005)==0x1005);
 c.write(0x8c,1);c.write(0x9c,1);c.write(0xba,3);c.write(0xb6,1);
 c.write(0xd6,0x80);
 std::array<u8,32> p{};
 le32(p,0,1U|(1U<<3)|(1U<<18)|(1U<<31)); // valid, 1 byte, max packet 1, EP1
 le32(p,4,(2U<<3)|(1U<<10)|(3U<<12)); // addr 2, IN, interrupt
 le32(p,8,u32((0x1000-0x400)/8)<<8);
 le32(p,12,0x80000000U); // active, DATA0
 host_write(c,0x800,p.data(),p.size());
 c.write(0xae,0x8000);c.write(0xac,0xfffe);
 auto x=[&](neo_usb::transaction const &tx,const u8 *out,u8 *in,u16 len){
   return hub.transfer(tx,out,in,len,[](u8){return false;});
 };
 assert(c.service_interrupt(x)==0);  // no real hub change -> NAK
 assert(c.read(0xaa)==0);
 hub.hub_changes[2]=0x10; // genuine PORT_RESET status-change
 assert(c.service_interrupt(x)==1);
 assert(c.read_host_word(0x1000)==0x0004); // bitmap: port 2 changed
 assert(c.read(0xaa)==1 && c.read(0xaa)==0);
 assert(c.read(0xac)&1);
 assert(c.read(0xd4)&0x80);
 assert(c.controller_interrupt());
 c.write(0xd4,0x80); // hardware W1C
 assert(!(c.read(0xd4)&0x80));
 assert(!c.controller_interrupt());
 std::cout<<"neo_usb_int=PASS INT PTD interrupt hub change; NAK/ACK, SRAM, W1C IRQ\n";
}
