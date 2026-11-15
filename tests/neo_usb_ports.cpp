// license:BSD-3-Clause
// Integrated USB controller test: original ISP1763 ATL PTD32, Dana-derived
// packet printer, and PC-port endpoint FIFOs without fabricated descriptors.
#include "neo_isp1763.h"
#include "neo_usb_function.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;
static void memory(neo::isp1763 &c,unsigned addr,const u8 *b,unsigned length){
    c.write(0xc4,u16(addr));
    for(unsigned n=0;n<length;n+=2)
        c.write(0xc6,u16(b[n])|u16(n+1<length?b[n+1]:0)<<8);
}
static void ptd(neo::isp1763 &c,u8 pid,u8 addr,u8 ep,u16 bytes,bool toggle){
    const u32 dw0=1U|(u32(bytes)<<3)|(64U<<18)|(u32(ep&1)<<31);
    const u32 dw1=(u32((ep>>1)&7))|(u32(addr&0x7f)<<3)|(u32(pid)<<10)
        |(u32(ep?2:0)<<12);
    const u32 dw2=u32((0x1000-0x400)/8)<<8;
    const u32 dw3=0x80000000U|(toggle?0x02000000U:0);
    const u32 d[4]={dw0,dw1,dw2,dw3};
    std::array<u8,32> bytes_le{};
    for(unsigned i=0;i<4;++i)
        for(unsigned b=0;b<4;++b)bytes_le[i*4+b]=u8(d[i]>>(8*b));
    memory(c,0x0c00,bytes_le.data(),bytes_le.size());
    c.write(0xb2,0xfffe);
    c.write(0xb4,1);
}
int main(){
    neo::isp1763 c;
    neo_usb::usb_function printer{};
    printer.kind=1;
    std::string paper;
    auto trans=[&](neo_usb::transaction const &tx,const u8 *out,u8 *in,u16 len){
        return printer.transfer(tx,out,in,len,[&](u8 ch){
            paper.push_back(char(ch));return true;
        });
    };
    const u8 set_address[8]={0,5,1,0,0,0,0,0};
    memory(c,0x1000,set_address,sizeof(set_address));
    ptd(c,2,0,0,8,false);
    c.write(0xa0,0x1000);c.write(0x8c,1);
    c.write(0x9c,1);c.write(0xba,1);c.write(0xb6,1);
    assert(c.service_host(trans)==0); // no physical printer mounted
    assert(c.read(0xb0)==0);
    c.set_host_attached(true);
    assert(c.read(0xa0)&1);
    assert(c.service_host(trans)==1);
    assert(c.read(0xb0)==1 && c.read(0xb0)==0);
    assert(printer.address==0);
    ptd(c,1,0,0,0,true); // control status IN, address latched on ACK
    assert(c.service_host(trans)==1);
    assert(printer.address==1);
    const u8 set_config[8]={0,9,1,0,0,0,0,0};
    memory(c,0x1000,set_config,sizeof(set_config));
    ptd(c,2,1,0,8,false);
    assert(c.service_host(trans)==1);
    ptd(c,1,1,0,0,true);
    assert(c.service_host(trans)==1);
    assert(printer.configuration==1);
    const u8 text[]={'N','E','O',' ','O','K'};
    memory(c,0x1000,text,sizeof(text));
    ptd(c,0,1,1,sizeof(text),false);
    assert(c.service_host(trans)==1);
    assert(paper=="NEO OK");
    assert(c.read(0xd4)&0x100);
    c.write(0xd4,0x100);
    assert(!(c.read(0xd4)&0x100));
    c.set_host_attached(false);
    assert(!(c.read(0xa0)&1));
    ptd(c,0,1,1,sizeof(text),true);
    assert(c.service_host(trans)==0); // no printer => no ACK
    // Independent upstream/PC port. Descriptors come from the guest firmware.
    c.write(0x0c,1); // NEO firmware enables DC mode
    assert(c.upstream(1,0,0,0,nullptr,nullptr,0).status==neo_usb::handshake::ack);
    assert(c.vbus());
    assert(c.upstream(3,0,0,0,nullptr,nullptr,0).status==neo_usb::handshake::ack);
    const u8 get_device[8]={0x80,6,0,1,0,0,18,0};
    assert(c.upstream(7,0,0,0,get_device,nullptr,8).status==neo_usb::handshake::ack);
    c.write(0x2c,0x20);
    assert(c.read(0x1c)==8);
    assert(c.read(0x20)==0x0680);
    c.write(0x28,0x10); // guest clears SETUP FIFO
    c.write(0x2c,0x01); // EP0 IN FIFO
    c.write(0x20,0x4241);
    c.write(0x28,0x08); // firmware validates TX
    std::array<u8,64> data{};
    assert(c.upstream(9,0,0,0,nullptr,data.data(),64).status==neo_usb::handshake::ack);
    assert(data[0]=='A'&&data[1]=='B');
    assert(c.upstream(9,0,0,0,nullptr,data.data(),64).status==neo_usb::handshake::nak);
    c.write(0x00,0x0081); // guest changes address after control status
    assert(c.device_address()==1);
    assert(c.upstream(3,0,0,0,nullptr,nullptr,0).status==neo_usb::handshake::ack);
    assert(c.device_address()==0);
    assert(c.upstream(2,0,0,0,nullptr,nullptr,0).status==neo_usb::handshake::ack);
    assert(!c.vbus());
    std::cout<<"neo_usb_ports=PASS host ATL SET_ADDRESS/SET_CONFIG/printer bulk bytes="
             <<paper.size()<<" device PC SETUP/IN/address/reset/disconnect\n";
}
