// license:BSD-3-Clause
// Adapted hardware-level SPI register and FIFO regression from Dana.
#include "neo_vz_spi.h"
#include <cassert>
#include <iostream>
int main(){
 neo::vz_spi1 p;
 assert(p.read(2)==0 && p.read(3)==1);
 p.write(2,0x6707);
 p.write(1,0x00a5);
 assert(p.read(1)==0x00a5 && p.read(4)==1);
 p.write(3,0x0800);
 p.write(2,0x6707); // XCH rising edge starts queued transfer
 assert(p.busy);
 unsigned bits=0;
 for(;p.busy && bits<17;++bits) p.shift_bit(true);
 assert(bits==8 && p.read(4)==0x0010);
 assert(p.irq());
 assert(p.read(0)==0x00ff);
 assert(!p.irq());
 assert(p.read(4)==0);
 assert((p.read(2)&0x0100)==0);
 p.write(2,0x0000);
 assert(!p.busy && p.read(4)==0);
 neo::vz_spi2 q;
 assert(q.read(0)==0 && q.read(1)==0);
 q.write(0,0x1265);
 q.write(1,0x0307);
 assert(q.busy);
 q.complete();
 assert(!q.busy && (q.read(1)&0x0180)==0x0080);
 assert((q.read(0)&0xff)==0xff);
 assert(q.transfers==1);
 q.write(1,0x03c7);
 assert(q.busy);
 q.complete(0x12);
 assert(q.irq());
 std::cout<<"neo_vz_spi=PASS VZ SPI1 FIFO/IRQ and SPI2 shift, open MISO\n";
}
