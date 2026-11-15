// license:BSD-3-Clause
// Native C++ regression of the real ISP1763A register/byte layout.
// Controller has no phantom attached host/printer.
#include "neo_isp1763.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
int main(){
 neo::isp1763 c;
 assert(c.read(0x70)==0x6320 && c.read(0x72)==0x0017);
 assert(c.read(0x02)==0xffff); // unknown MMIO must not silently succeed
 assert(c.read(0xc6)==0); // initially empty HC SRAM
 c.write(0xc4,0x0c00);
 c.write(0xc6,0x1234);
 c.write(0xc6,0xabcd);
 c.write(0xc4,0x0c00);
 assert(c.read(0xc6)==0x1234);
 assert(c.read(0xc6)==0xabcd);
 c.write(0xc4,0x5ffe);
 c.write(0xc6,0xbeef);
 c.write(0xc4,0x5ffe);
 assert(c.read(0xc6)==0xbeef);
 assert(c.read(0xc6)==0xffff); // reject beyond 24KiB
 c.write(0xd6,0x0380);assert(c.read(0xd6)==0x0380);
 c.write(0xd4,0xffff);assert(c.read(0xd4)==0x40);
 c.write(0x14,0xffff);c.write(0x16,0xffff);
 c.write(0x2c,0x20); // EP0 SETUP isolated from EP0 OUT/IN
 std::array<std::uint8_t,8> pkt{0x80,6,0,1,0,0,18,0};
 assert(c.inject_out(16,pkt.data(),8));
 assert(c.read(0x1c)==8);
 assert(c.read(0x20)==0x0680);
 assert(c.read(0x20)==0x0100);
 assert(c.read(0x18)&0x100);
 assert(c.irq());
 c.write(0x18,0x100);assert(!(c.read(0x18)&0x100));
 c.write(0x2c,0x00);assert(c.read(0x1c)==0);
 c.write(0x20,0x55aa);c.write(0x28,0x08);
 c.write(0x2c,0x01); // distinct EP0 IN index, not same FIFO
 assert(c.read(0x1c)==0);
 std::array<std::uint8_t,64> dat{};std::uint16_t count=64;
 assert(!c.take_in(1,dat.data(),count));
 c.write(0x2c,0x00);count=64;
 assert(c.take_in(0,dat.data(),count));
 assert(count==2 && dat[0]==0xaa && dat[1]==0x55);
 c.write(0x2c,0x00);c.write(0x04,0x0200); // 512B buffer reallocated safely
 c.write(0x2c,0x01);c.write(0x20,0x4455);
 c.write(0x2c,0x00);c.write(0x20,0x1234);c.write(0x28,0x08);
 c.write(0x2c,0x01);c.write(0x28,0x08);count=64;
 assert(c.take_in(1,dat.data(),count));
 assert(count==2 && dat[0]==0x55 && dat[1]==0x44);
 c.write(0x2c,0x00);count=64;
 assert(c.take_in(0,dat.data(),count));
 assert(count==2 && dat[0]==0x34 && dat[1]==0x12);
 c.write(0xb8,1);assert(c.read(0x70)==0x6320 && c.read(0x14)==0 && c.read(0xd6)==0);
 std::cout<<"neo_isp1763=PASS (ID MMIO, HC 24KiB, DC 4KiB, endpoints, interrupt W1C, reset)\n";
}
