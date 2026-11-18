// license:BSD-3-Clause
// copyright-holders:DANA-emu contributors, SamDelorean contributors
// MC68VZ328 PWM1/PWM2 registers and FIFO, adapted from DANA-emu.
#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
namespace neo {
struct vz_pwm {
 using u8=std::uint8_t;using u16=std::uint16_t;
 u16 control=0x0020,sample=0,control2=0,period2=0,width2=0,count2=0;
 u8 period=0xfe,counter=0,last_sample=0,head=0,count=0,repeat=0;
 std::array<u8,5> fifo{};
 bool high=false;
 std::uint64_t cycles=0,consumed=0;
 void reset(){*this=vz_pwm{};}
 static u16 merge(u16 old,u16 v,u16 mask){return (old&~mask)|(v&mask);}
 bool enabled()const{return bool(control&0x10);}
 bool irq()const{return (control&0xc0)==0xc0;}
 unsigned repeat_count()const{return 1U<<((control>>2)&3);}
 unsigned period_steps()const{return unsigned(period)+2;}
 unsigned divider()const{return unsigned(((control>>8)&127)+1)*(2U<<(control&3));}
 u16 read(unsigned offset)const{
  switch(offset){
  case 0:return (control&~u16(0x20))|u16(count<5?0x20:0);
  case 2:return sample;
  case 4:return (u16(period)<<8)|counter;
  case 0x10:return control2;
  case 0x12:return period2;
  case 0x14:return width2;
  case 0x16:return count2;
  default:return 0xffff;
  }
 }
 void push(u8 n){if(count<5){fifo[(head+count)%5]=n;++count;}}
 void write(unsigned offset,u16 data,u16 mask=0xffff){
  switch(offset){
  case 0:{
    const u16 before=control;
    control=merge(control,data,mask)&0xffdf;
    if(data&mask&0x80)control|=0x80;
    if(!(control&0x10)){
     head=count=repeat=last_sample=counter=0;high=false;
     if(before&0x10)control&=~u16(0x80);
    }else if(!(before&0x10)){head=count=repeat=last_sample=counter=0;control|=0x80;}
    return;
  }
  case 2:
    if(!enabled()||count==5)return;
    sample=merge(sample,data,mask);
    if(mask==0xffff)push(u8(data>>8));
    if(mask&0xff)push(u8(data));
    if(count>1)control&=~u16(0x80);
    return;
  case 4:if(mask&0xff00)period=u8(data>>8);return;
  case 0x10:control2=merge(control2,data,mask);return;
  case 0x12:period2=merge(period2,data,mask);return;
  case 0x14:width2=merge(width2,data,mask);return;
  case 0x16:return; // hardware counter is read-only
  default:return;
  }
 }
 // Advance at the actual PWM period timer boundary. The falling edge is
 // scheduled by the MAME carrier at min(sample,period) clock steps.
 void period_tick(){
  if(!enabled()){high=false;return;}
  ++cycles;counter=0;
  if(repeat)--repeat;
  else{
    if(count){
      last_sample=fifo[head];head=(head+1)%5;--count;++consumed;
      repeat=u8(repeat_count()-1);
    }
    if(count<=1)control|=0x80;
  }
  high=last_sample>0;
 }
 void fall(){high=false;}
 bool pwm2_enabled()const{return bool(control2&0x0001);}
 void pwm2_tick(){
   if(!pwm2_enabled())return;
   count2=u16(count2+1);
   if(period2&&count2>=period2)count2=0;
 }
};
} // namespace neo
