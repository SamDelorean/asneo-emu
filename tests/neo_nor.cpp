// license:BSD-3-Clause
// NOR x16 chip protocol, geometry, and persistence of the raw virtual media.
// Uses synthetic bytes; never ships original NEO software or external dumps.
#include "neo_nor.h"
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <vector>
int main(){
    std::vector<std::uint8_t> medium(neo::nor_flash::size_bytes,0xff);
    neo::nor_flash flash;
    flash.attach(medium.data(),medium.size());
    assert(flash.ready());
    assert(flash.read_only());
    assert(!flash.write_word(0xaaa,0x00aa)); // unknown chip is safely read-only
    assert(!flash.write_word(0x00aa,0x9898,0xff00)); // no invented CFI for unknown chip
    flash.select_profile(neo::nor_flash::profile::m29w320eb);
    assert(!flash.read_only());
    auto seq=[&](std::uint16_t cmd){
        assert(flash.write_word(0xaaa,0x00aa));
        assert(flash.write_word(0x554,0x0055));
        assert(flash.write_word(0xaaa,cmd));
    };
    // AMD 16-bit unlock and Autoselect; ST bottom-boot IDs.
    seq(0x0090);
    assert(flash.read_word(0)==0x20);
    assert(flash.read_word(2)==0x2257);
    assert(flash.write_word(0,0xf0));
    // Real CFI read query address word 0x55.
    assert(flash.write_word(0x00aa,0x0098));
    assert(flash.read_word(0x20)=='Q');
    assert(flash.read_word(0x22)=='R');
    assert(flash.read_word(0x24)=='Y');
    assert(flash.read_word(0x26)==2); // AMD-compatible command set
    assert(flash.read_word(0x4e)==0x16); // 4MiB
    assert(flash.read_word(0x58)==2); // 2 erase regions
    assert(flash.write_word(0,0xf0));
    assert(flash.read_word(0x20)==0xffff);
    // NEO 2013's actual low-level geometry probe uses byte-wide QRY enter
    // at even $4000AA (upper byte lane) and reads QRY via odd addresses.
    // Its 0x9898 word data is a MAME lane-replicated MOVE.B, not a word cmd.
    assert(flash.write_word(0x00aa,0x9898,0xff00));
    assert(flash.current_state()==neo::nor_flash::state::query);
    assert((flash.read_word(0x20)&0x00ff)==0x51);
    assert((flash.read_word(0x22)&0x00ff)==0x52);
    assert((flash.read_word(0x24)&0x00ff)==0x59);
    assert(flash.read_word(0x4e)==0x16); // 4MiB CFI geometry
    assert(flash.write_word(0x00aa,0xf0f0,0xff00));
    assert(flash.current_state()==neo::nor_flash::state::array);
    assert(flash.read_word(0x20)==0xffff);
    // Do not accidentally broaden permissions to arbitrary byte commands.
    const auto preserved=flash.read_word(0x10000);
    assert(!flash.write_word(0x0aaa,0xaaaa,0xff00));
    assert(!flash.write_word(0x0554,0x5555,0xff00));
    assert(flash.read_word(0x10000)==preserved);
    // Program a word, enforce NOR one-way clearing (can't set bits to 1).
    seq(0x00a0);
    assert(flash.write_word(0x10000,0x1234));
    assert(flash.read_word(0x10000)==0x1234);
    seq(0x00a0);
    assert(!flash.write_word(0x10000,0xffff));
    assert(flash.read_word(0x10000)==0x1234);
    // Verify failed unlock cannot change original data.
    assert(!flash.write_word(0x554,0x55));
    assert(flash.read_word(0x10000)==0x1234);
    // Sector erase exact block geometry, with adjacent 8KiB and 64KiB guard.
    auto erase=[&](std::size_t address) {
        assert(flash.write_word(0xaaa,0xaa));
        assert(flash.write_word(0x554,0x55));
        assert(flash.write_word(0xaaa,0x80));
        assert(flash.write_word(0xaaa,0xaa));
        assert(flash.write_word(0x554,0x55));
        assert(flash.write_word(address,0x30));
    };
    medium[0x1fff]=0x00;medium[0x2000]=0x00;
    erase(0x0004);
    assert(medium[0x1fff]==0xff);
    assert(medium[0x2000]==0x00);
    medium[0x10000]=0x01;
    medium[0x1ffff]=0x00;medium[0x20000]=0x00;
    erase(0x10000);
    assert(medium[0x10000]==0xff && medium[0x1ffff]==0xff);
    assert(medium[0x20000]==0x00);
    // Flash reset does not reset/purge NOR bytes: MAME NVRAM has stable base.
    seq(0xa0);
    assert(flash.write_word(0x20000,0x0055));
    const auto saved=medium;
    flash.reset();
    assert(flash.read_word(0x20000)==0x0055);
    flash.attach(medium.data(),medium.size());
    flash.select_profile(neo::nor_flash::profile::m29w320eb);
    assert(medium==saved);
    // Program data 0x00F0 must not be mistaken for a reset command.
    seq(0xa0);
    assert(flash.write_word(0x38000,0x00f0));
    assert(flash.read_word(0x38000)==0x00f0);
    // Chip erase requires full six-step sequence, not an arbitrary 0x10.
    assert(!flash.write_word(0xaaa,0x10));
    assert(flash.read_word(0x20000)==0x0055);
    erase(0x20000);
    assert(flash.read_word(0x20000)==0xffff);
    assert(!flash.write_word(0xaaa,0x00aa,0x00ff)); // reject unknown byte lanes
    // Unlock bypass mode for single word programming (datasheet).
    seq(0x20);
    assert(flash.write_word(0x1234,0xa0));
    assert(flash.write_word(0x30000,0x0f0f));
    assert(flash.read_word(0x30000)==0x0f0f);
    assert(flash.write_word(0x1234,0x90));
    assert(flash.write_word(0x1234,0x00));
    assert(flash.current_state()==neo::nor_flash::state::array);
    // The AMD chip erase sixth write is 0x10 at unlock word 0x555.
    seq(0x80); // arm erase
    assert(flash.write_word(0xaaa,0xaa));
    assert(flash.write_word(0x554,0x55));
    assert(flash.write_word(0xaaa,0x10));
    assert(flash.read_word(0x30000)==0xffff);
    assert(flash.read_word(0x38000)==0xffff);
}
