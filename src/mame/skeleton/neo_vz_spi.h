// license:BSD-3-Clause
// copyright-holders:DANA-emu contributors, SamDelorean contributors
// MC68VZ328 SPI1/SPI2 peripheral cores. The SPI1 FIFO/status contract is
// adapted from DANA-emu's qualified dana_sd::controller; no Dana SD card,
// wireless chip, or fabricated external response is present in NEO.
#pragma once
#include <array>
#include <cstdint>
#include <algorithm>
namespace neo {
struct vz_spi1 {
 using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;
 std::array<u16,8> tx{},rx{};
 u16 control=0,mask=0,spc=0,last_tx=0,last_rx=0,received=0,word=0;
 u8 tx_head=0,rx_head=0,tx_count=0,rx_count=0,bits_left=0;
 bool rx_edge=false,busy=false,overflow=false;
 u32 words_sent=0,words_received=0;
 void reset(){*this=vz_spi1{};}
 u8 flags() const {
   return (rx_edge?0x40:0)|(rx_count==8?0x20:0)|(rx_count>=4?0x10:0)|
     (rx_count?8:0)|(tx_count==8?4:0)|(tx_count>=4?2:0)|(!tx_count?1:0);
 }
 bool irq() const {return bool(flags() & (mask>>8));}
 u16 read(unsigned reg,bool effects=true){
   switch(reg){
   case 0:{const u16 value=rx_count?rx[rx_head]:last_rx;
     if(effects&&rx_count){rx_head=(rx_head+1)&7;--rx_count;last_rx=value;}
     return value;}
   case 1:return last_tx;
   case 2:return control;
   case 3:{const u16 result=mask|flags();if(effects)rx_edge=false;return result;}
   case 4:return u16(tx_count)|(u16(rx_count)<<4);
   case 5:return spc;
   default:return 0xffff;
   }
 }
 static u16 combine(u16 old,u16 value,u16 lane){return (old&~lane)|(value&lane);}
 void write(unsigned reg,u16 data,u16 lane=0xffff){
   switch(reg){
   case 0:return;
   case 1:
     last_tx=combine(last_tx,data,lane);
     if(!(control&0x0200))return;
     if(tx_count==8){overflow=true;return;}
     tx[(tx_head+tx_count)&7]=last_tx;++tx_count;return;
   case 2:{
     u16 prev=control;control=combine(control,data,lane);
     if(!(control&0x0200)){tx_count=rx_count=tx_head=rx_head=0;rx_edge=false;}
     if((control&0x0700)!=0x0700){busy=false;control&=~u16(0x0100);}
     else if(!(prev&0x0100))start();
     return;
   }
   case 3:mask=combine(mask,data,lane)&0x7f00;return;
   case 4:return; // SPITEST is a diagnostic-only register
   case 5:spc=combine(spc,data,lane);return;
   default:return;
   }
 }
 unsigned bits()const{return (control&15)+1;}
 unsigned bit_divisor()const{return 4U<<((control>>13)&7);}
 void start(){
   if(!tx_count || (control&0x0700)!=0x0700){busy=false;control&=~u16(0x0100);return;}
   word=tx[tx_head];tx_head=(tx_head+1)&7;--tx_count;
   received=0;bits_left=u8(bits());busy=true;
 }
 // Returns true only when a whole word has completed. The unattached
 // RF pins have a pull-up on MISO, never an invented CC2430 response.
 bool shift_bit(bool miso_high=true){
   if(!busy||!bits_left)return false;
   received=u16((received<<1)|(miso_high?1U:0U));
   --bits_left;
   if(bits_left)return false;
   ++words_sent;
   if(rx_count<8){
     if(!rx_count)rx_edge=true;
     rx[(rx_head+rx_count)&7]=received;++rx_count;++words_received;
   }else overflow=true;
   start();
   return true;
 }
};
struct vz_spi2 {
 using u16=std::uint16_t;using u32=std::uint32_t;
 u16 data=0,control=0;
 u32 transfers=0;
 bool busy=false;
 void reset(){*this=vz_spi2{};}
 u16 read(unsigned index)const{return index?control:data;}
 static u16 combine(u16 old,u16 v,u16 lane){return (old&~lane)|(v&lane);}
 void write(unsigned index,u16 value,u16 lane=0xffff){
   if(!index){data=combine(data,value,lane);return;}
   control=combine(control,value,lane);
   if((control&0x0300)==0x0300)busy=true;
 }
 void complete(u16 external=0xffff){
   if(!busy)return;
   const unsigned width=(control&15)+1;
   const u16 mask=width>=16?0xffff:u16((1U<<width)-1);
   data=(data&~mask)|(external&mask);
   busy=false;control=(control&~u16(0x0100))|0x0080;
   ++transfers;
 }
 bool irq()const{return bool((control&0xc0)==0xc0);}
};
} // namespace neo
