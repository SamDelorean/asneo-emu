// license:BSD-3-Clause
// Tests CPU-independent VZ register/timer/RTC/IRQ contract adapted from Dana.
#include "neo_vz_core.h"
#include <cassert>
#include <cstdint>
int main() {
    neo::vz_core c;
    assert(c.read_word(0xf200).has_value());
    assert(c.pll_hz() == 2U*(14U*(0x47+1)+3+1)*32768U);
    assert(c.read_word(0xf304).value() == 0x00ff);
    assert(c.read_word(0xf306).value() == 0xffff);
    // MC68VZ328 timer1: SYSCLK, enabled, interrupt enabled, cmp 4.
    assert(c.write_word(0xf602,0,0xffff));
    assert(c.write_word(0xf604,4,0xffff));
    assert(c.write_word(0xf600,0x0013,0xffff));
    assert(c.timer_source_hz(0) == c.sysclk_hz());
    assert(!c.irq_at_level(6)); // timer masked
    assert(c.write_word(0xf306,0xfffd,0xffff)); // unmask timer1 IRQ bit1
    c.compare(0);
    assert(c.timer_state(0).status & 1);
    assert(c.irq_at_level(6));
    assert(c.irq_vector(6)==6);
    assert(c.read_word(0xf60a).value()&1);
    assert(c.write_word(0xf60a,0xffff,0xffff));
    assert(!c.irq_at_level(6));
    // 2nd timer interrupt route selected by ILCR bits[2:0] -> level 3.
    assert(c.write_word(0xf314,3,0xffff));
    assert(c.write_word(0xf306,0xffdf,0xffff));
    assert(c.write_word(0xf610,0x0019,0xffff));
    c.compare(1);
    assert(c.irq_at_level(3));
    // Clock control shares real Dana PLL equation and obeys lane writes.
    const auto old=c.sysclk_hz();
    assert(c.write_word(0xf200,0x0100,0x0700));
    assert(c.sysclk_hz()!=old);
    assert(c.write_word(0xf202,0x4000,0x4000));
    const auto fsr=c.read_word(0xf202).value()&0x0fff;
    c.write_word(0xf202,0x0a00,0x0f00);
    assert((c.read_word(0xf202).value()&0x0fff)==fsr);
    // RTC: actual crystal edges, not fake millisecond-based wall clock.
    c.seed_rtc(23,59,59);
    c.advance_clk32(32768);
    assert(c.read_word(0xfb00).value()==0);
    assert(c.read_word(0xfb02).value()==0);
    assert(c.read_word(0xfb1a).value()==1);
    // Alarm IRQ after enabling pending low status bits.
    assert(c.write_word(0xfb10,0x0010,0xffff));
    assert(c.write_word(0xf306,0xffef,0xffff));
    assert(c.irq_at_level(4));
    assert(!c.write_word(0xf88e,0,0xffff)); // no fabricated unimplemented peripheral
}
