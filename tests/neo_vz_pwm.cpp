// license:BSD-3-Clause
#include "neo_vz_pwm.h"
#include <cassert>
#include <iostream>
int main(){
 neo::vz_pwm p;
 assert(p.read(0)==0x20 && p.read(4)==0xfe00);
 p.write(0,0x0010);
 p.write(2,0x2010); // 2 samples, high byte then low byte
 assert(p.count==2);
 p.period_tick();
 assert(p.last_sample==0x20 && p.high && p.count==1);
 p.fall();assert(!p.high);
 p.period_tick();
 assert(p.last_sample==0x10 && p.count==0);
 p.write(0,0x00d0); // IRQ enabled and FIFO empty
 assert(p.irq());
 p.write(0,0x0000);
 assert(!p.enabled()&&!p.high&&p.count==0);
 p.write(0x10,1);
 p.write(0x12,4);
 p.write(0x14,2);
 for(int n=0;n<4;++n)p.pwm2_tick();
 assert(p.count2==0 && p.pwm2_enabled());
 std::cout<<"neo_vz_pwm=PASS PWM1 FIFO/sample timing & PWM2 counters\n";
}
