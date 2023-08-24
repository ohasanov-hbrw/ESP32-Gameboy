#include <GBRam.h>


typedef struct{
    u8 wram[0x2000];
    u8 hram[0x80];
}ramContext;

static ramContext RAM;

void cleanRam(){
    memset(RAM.wram, 0, sizeof RAM.wram);
    memset(RAM.hram, 0, sizeof RAM.hram);
}

u8 readFromWram(u16 address) {
    address -= 0xC000;
    if(address >= 0x2000){
        printf("\tERR: INV WRAM ADDR %08X\n", address + 0xC000);
        exit(-1);
    }
    return RAM.wram[address];
}

void writeToWram(u16 address, u8 value) {
    address -= 0xC000;
    RAM.wram[address] = value;
}

u8 readFromHram(u16 address) {
    address -= 0xFF80;
    return RAM.hram[address];
}

void writeToHram(u16 address, u8 value) {
    address -= 0xFF80;
    RAM.hram[address] = value;
}