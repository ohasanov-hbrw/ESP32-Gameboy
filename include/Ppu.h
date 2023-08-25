#pragma once

#include <Utils.h>

static const int LINES_PER_FRAME = 154;
static const int TICKS_PER_LINE = 456;
static const int YRES = 144;
static const int XRES = 160;

typedef enum{
    FS_TILE,
    FS_DATA0,
    FS_DATA1,
    FS_IDLE,
    FS_PUSH
}fetchState;

typedef struct _fifoEntry{
    struct _fifoEntry *next;
    u32 value;
}fifoEntry;

typedef struct{
    fifoEntry *head;
    fifoEntry *tail;
    u32 size;
}fifo;

typedef struct{
    fetchState currentFetchState;
    fifo pixelFifo;
    u8 lineX;
    u8 pushedX;
    u8 fetchedX;
    u8 bgwFetchData[3];
    u8 entryFetchData[6];
    u8 mayY;
    u8 mapX;
    u8 tileY;
    u8 fifoX;
}pixelFifoContext;


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
    pixelFifoContext pfc;
    u32 currentFrame;
    u32 lineTicks;
    u32* vBuffer;
}ppuContext;


ppuContext* getPpuContext();

void initPpu();
void stepPpu();

void writeOam(u16, u8);
u8 readOam(u16);

void writeVram(u16, u8);
u8 readVram(u16);

void processPipeline();

void pipelineFifoReset();