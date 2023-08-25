#pragma once

#include <Utils.h>

typedef struct{
    u8 y;
    u8 x;
    u8 tile;
    unsigned fcgbpn : 3;
    unsigned fcgbv : 1;
    unsigned fpn : 1;
    unsigned fxflip : 1;
    unsigned fyflip : 1;
    unsigned fbgp : 1;
}oamEntry;

typedef struct{
    oamEntry oamRam[40];
    u8 vram[0x2000];
}ppuContext;


ppuContext* getPpuContext();

void initPpu();
void stepPpu();

void writeOam(u16, u8);
u8 readOam(u16);

void writeVram(u16, u8);
u8 readVram(u16);