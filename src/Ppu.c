#include <Ppu.h>

static ppuContext PPU = {0};

void initPpu(){

}

void stepPpu(){

}

void writeOam(u16 address, u8 value){
    if(address >= 0xFE00){
        address -= 0xFE00;
    }
    u8* p = (u8*)PPU.oamRam;
    p[address] = value;

}
u8 readOam(u16 address){
    if(address >= 0xFE00){
        address -= 0xFE00;
    }
    u8* p = (u8*)PPU.oamRam;
    return p[address];
}

void writeVram(u16 address, u8 value){
    PPU.vram[address - 0x8000] = value;
}

u8 readVram(u16 address){
    return PPU.vram[address - 0x8000];
}



ppuContext* getPpuContext(){
    return &PPU;
}