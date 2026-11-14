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
    std::vector<std::uint8_t> os(0x80,0);
    std::memcpy(os.data()+6,"System 3 Neo      ",18);
    be32(os,0x50,0x00410000);be32(os,0x54,0x10);
    for(unsigned i=0;i<0x10;++i)os[0x70+i]=i;
    assert(neo::firmware::install_system(os.data(),os.size(),flash,err));
    assert(flash[0x10000]==0 && flash[0x1000f]==15);
    os[0x50]=0x20;
    assert(!neo::firmware::install_system(os.data(),os.size(),flash,err));
    assert(flash[0x1000f]==15); // failed installs cannot partially modify Flash
}
