// license:BSD-3-Clause
// Native USB 1.1 hub / downstream printer enumeration regression.
// No firmware patches, synthetic print data, radio or external network.
#include "neo_usb_function.h"
#include <array>
#include <cassert>
#include <iostream>
#include <string>
using u8=std::uint8_t;
static neo_usb::reply x(neo_usb::usb_function &dev,
                       u8 address,neo_usb::pid pid,u8 endpoint,
                       const u8 *out,u8 *in,std::uint16_t count,bool toggle=false) {
  neo_usb::transaction t{address,endpoint,pid,false,false,toggle,64};
  return dev.transfer(t,out,in,count,[](u8){return true;});
}
static void ctl_setup(neo_usb::usb_function &dev,u8 addr,
                      const std::array<u8,8>&p) {
  assert(x(dev,addr,neo_usb::pid::setup,0,p.data(),nullptr,8).status==neo_usb::handshake::ack);
}
static void ctl_status_in(neo_usb::usb_function &dev,u8 addr) {
  u8 empty=0;
  auto r=x(dev,addr,neo_usb::pid::in,0,nullptr,&empty,0,true);
  assert(r.status==neo_usb::handshake::ack && r.size==0);
}
static void ctl_status_out(neo_usb::usb_function &dev,u8 addr){
  auto r=x(dev,addr,neo_usb::pid::out,0,nullptr,nullptr,0,true);
  assert(r.status==neo_usb::handshake::ack);
}
int main(){
  neo_usb::usb_function hub{},printer{};
  hub.kind=3;printer.kind=1;hub.hub_attach(true);
  ctl_setup(hub,0,{{0,5,2,0,0,0,0,0}});
  ctl_status_in(hub,0);
  assert(hub.address==2);
  ctl_setup(hub,2,{{0x80,6,0,1,0,0,18,0}});
  std::array<u8,64> data{};
  auto r=x(hub,2,neo_usb::pid::in,0,nullptr,data.data(),18,true);
  assert(r.status==neo_usb::handshake::ack&&r.size==18&&data[4]==9);
  ctl_status_out(hub,2);
  ctl_setup(hub,2,{{0,9,1,0,0,0,0,0}});
  assert(hub.configuration==0); // never configure from SETUP alone
  // This is the precise status PTD sequence from original NEO13 ROM:
  // request 00 09 01... followed by zero-length OUT, not IN.
  ctl_status_out(hub,2);
  assert(hub.configuration==1);
  ctl_setup(hub,2,{{0xa0,6,0,0x29,0,0,15,0}});
  r=x(hub,2,neo_usb::pid::in,0,nullptr,data.data(),15,true);
  assert(r.status==neo_usb::handshake::ack&&r.size==9&&data[1]==0x29&&data[2]==2);
  ctl_status_out(hub,2);
  ctl_setup(hub,2,{{0x23,3,8,0,2,0,0,0}});
  ctl_status_in(hub,2);
  assert(!hub.hub_child_enabled());
  ctl_setup(hub,2,{{0x23,3,4,0,2,0,0,0}});
  ctl_status_in(hub,2);
  for(int i=0;i<9;++i){hub.hub_tick_1ms();assert(!hub.hub_child_enabled());}
  hub.hub_tick_1ms();
  assert(hub.hub_child_enabled());
  ctl_setup(hub,2,{{0xa3,0,0,0,2,0,4,0}});
  r=x(hub,2,neo_usb::pid::in,0,nullptr,data.data(),4,true);
  assert(r.status==neo_usb::handshake::ack&&r.size==4);
  assert((data[0]&3)==3&&data[1]==1&&data[2]==0x11);
  ctl_status_out(hub,2);
  ctl_setup(hub,2,{{0x23,1,20,0,2,0,0,0}});
  ctl_status_in(hub,2);
  assert(!(hub.hub_changes[2]&0x10));
  printer.legacy_pcl_identity=true;
  ctl_setup(printer,0,{{0x80,6,0,1,0,0,18,0}});
  r=x(printer,0,neo_usb::pid::in,0,nullptr,data.data(),18,true);
  assert(r.status==neo_usb::handshake::ack&&r.size==18);
  assert(data[8]==0xf0 && data[9]==0x03 && data[10]==0x17 && data[11]==0x03);
  ctl_status_out(printer,0);
  ctl_setup(printer,0,{{0,5,3,0,0,0,0,0}});
  ctl_status_in(printer,0);
  assert(printer.address==3);
  ctl_setup(printer,3,{{0,9,1,0,0,0,0,0}});
  ctl_status_in(printer,3);
  assert(printer.configuration==1);
  std::string bytes;
  neo_usb::transaction tx{3,1,neo_usb::pid::out,false,false,false,64};
  const std::array<u8,8> payload{'O','S','3','K',' ','P','R','N'};
  auto reply=printer.transfer(tx,payload.data(),nullptr,8,
    [&](u8 b){bytes.push_back(char(b));return true;});
  assert(reply.status==neo_usb::handshake::ack&&bytes=="OS3K PRN");
  std::cout<<"neo_usb_hub=PASS 2-port hub, downstream printer enumeration & 8 bytes\n";
}
