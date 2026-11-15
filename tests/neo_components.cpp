// license:BSD-3-Clause
// Local standalone C++ component checks, no MAME or proprietary ROMs needed.
// Build from repository root: c++ -std=c++17 -Wall -Wextra -pedantic -Isrc/mame/skeleton tests/neo_components.cpp -o /tmp/neo_components && /tmp/neo_components
#include "neo_lcd.h"
#include "neo_keyboard.h"
#include "neo_firmware.h"
#include <cassert>
#include <cstring>
#include <vector>
#include <string>
static void be32(std::vector<std::uint8_t> &v,unsigned offset,std::uint32_t n) {
    v[offset]=n>>24;v[offset+1]=n>>16;v[offset+2]=n>>8;v[offset+3]=n;
}
int main() {
    neo::keyboard k;
    assert(k.columns()==0xff);
    k.gpio_write(0xf410,0x80); // row 0 via PC7
    assert(k.selected_rows() & 1);
    k.set_row(0,0xfe); // column 0 pressed
    assert(k.columns()==0xfe);
    k.gpio_write(0xf410,0x00);k.gpio_write(0xf408,0x20);
    assert(k.selected_rows()==(1u<<9));
    k.set_row(9,0xfb);assert(k.columns()==0xfb);
    k.gpio_write(0xf408,0);k.gpio_write(0xf440,0x80);
    assert(k.selected_rows()==(1u<<10));
    assert(neo::keyboard::logical(8,0)==0xff);
    assert(neo::keyboard::logical(0,0)==0x00);
    assert(neo::matrix::key_count()==80);
    // Full original 16x8 matrix, including all 48 unpopulated sites;
    // verify electrical decoding for all 80 keys, not a series of
    // single-key fixes. 0xff HID denotes two NEO-specific buttons.
    unsigned scanned=0,empties=0;
    for(unsigned row=0;row<16;++row) for(unsigned col=0;col<8;++col) {
        neo::keyboard single; single.reset();
        const auto logical=neo::keyboard::logical(row,col);
        if(logical==0xff) {++empties;assert(neo::keyboard::hid(row,col)==0xff);continue;}
        ++scanned;
        const auto mask=std::uint8_t(1U<<col);
        if(row<8)single.gpio_write(0xf410,std::uint8_t(0x80>>row));
        else if(row==9)single.gpio_write(0xf408,0x20);
        else if(row>=10)single.gpio_write(0xf440,std::uint8_t(0x80>>(row-10)));
        else assert(false && "Row 8 must be electrically empty");
        single.set_row(row,std::uint8_t(~mask));
        assert(single.selected_rows()==(1U<<row));
        assert(single.columns()==std::uint8_t(~mask));
        single.set_row(row,0xff);
        assert(single.columns()==0xff);
    }
    assert(scanned==80 && empties==48);
    // SmallROM direct row decode F411 before System3 masks take over.
    neo::keyboard small_scan;
    small_scan.gpio_write(0xf411,0x0d); // original T raw=0x0d
    small_scan.set_row(13,0xfe);
    assert(small_scan.columns()==0xfe);
    small_scan.gpio_write(0xf410,0x80); // now full System3 GPIO mode
    assert(small_scan.selected_rows()==1 && small_scan.columns()==0xff);
    small_scan.gpio_write(0xf411,0x0d); // no spurious override
    assert(small_scan.selected_rows()==1);
    // Exact firmware anchors and shifted modifiers, no guessed letters.
    assert(neo::keyboard::hid(10,3)==0x08); // E raw=0x3a
    assert(neo::keyboard::hid(13,3)==0x15); // R raw=0x3d
    assert(neo::keyboard::hid(15,7)==0x11); // N raw=0x7f
    assert(neo::keyboard::hid(0,3)==0x0c); // I raw=0x30
    assert(neo::keyboard::hid(9,6)==0x28); // Enter
    assert(neo::keyboard::hid(6,4)==0x58); // Applets
    assert(neo::keyboard::hid(7,4)==0xff); // Send: no HID bridge

    neo::lcd lcd;
    lcd.command(0,0xb0);lcd.command(0,0x00);lcd.write(0,0x01);
    assert(lcd.pixel(0,0));
    assert(!lcd.pixel(1,0));
    lcd.command(0,0x00);assert(lcd.read(0)==0x01);
    lcd.command(0,0xae);assert(!lcd.pixel(0,0));
    lcd.command(0,0xaf);lcd.command(0,0xa7);assert(!lcd.pixel(0,0));
    lcd.command(0,0xa6);assert(lcd.pixel(0,0));
    lcd.command(1,0xb0);lcd.command(1,0x00);lcd.write(1,0x01);
    assert(lcd.pixel(132,0));
    lcd.reset();assert(!lcd.pixel(132,0));
    std::vector<std::uint8_t> small(0x581a,0xff),flash(0x200000,0xff);
    be32(small,0,0x0007fff0);be32(small,4,0x0040042a);
    std::string err;
    assert(neo::firmware::install_small(small.data(),small.size(),flash,err));
    assert(flash[0]==0x00 && flash[4]==0x00 && flash[6]==0x04);
    // R6: complete source packet, including header, is transferred in
    // 1 KiB chunks. The second descriptor's nominal 0x14 bytes still occupy
    // a physical 0x400-byte transfer; do not shift the next segment.
    std::vector<std::uint8_t> os(0xc00,0);
    std::memcpy(os.data()+6,"System 3 Neo      ",18);
    be32(os,0x50,0x00410000);be32(os,0x54,0x400);
    be32(os,0x58,0x00406000);be32(os,0x5c,0x14);
    be32(os,0x60,0x005ffc00);be32(os,0x64,0x400);
    os[0x70]=0x5a;
    os[0x400]=0x53;os[0x413]=0x54;os[0x7ff]=0x3c;
    os[0x800]=0x91;os[0xbff]=0xa9;
    std::vector<neo::firmware::segment> segments;
    assert(neo::firmware::parse_system(os.data(),os.size(),segments,err));
    assert(segments.size()==3);
    assert(segments[0].offset==0 && segments[0].transfer_size==0x400);
    assert(segments[1].offset==0x400 && segments[1].size==0x14 &&
           segments[1].transfer_size==0x400);
    assert(segments[2].offset==0x800 && segments[2].transfer_size==0x400);
    assert(neo::firmware::install_system(os.data(),os.size(),flash,err));
    assert(flash[0x10000]==os[0] && flash[0x10070]==0x5a);
    assert(flash[0x6000]==0x53 && flash[0x6013]==0x54 && flash[0x63ff]==0x3c);
    assert(flash[0x1ffc00]==0x91 && flash[0x1fffff]==0xa9);
    os[0x50]=0x20; // invalid destination: no partial image writes
    assert(!neo::firmware::install_system(os.data(),os.size(),flash,err));
    assert(flash[0x1fffff]==0xa9);
}
