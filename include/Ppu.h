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
    fifo pixelFifoBackground;
    fifo pixelFifoSprite;
    u8 lineX;
    u8 pushedX;
    u8 fetchedX;
    u8 bgwFetchData[3];
    u8 entryFetchData[6];
    u16 mapY;
    u16 mapX;
    u8 tileY;
    u8 fifoX;
}pixelFifoContext;


typedef struct{
    u8 y;
    u8 x;
    u8 tile;
    u8 flags;
    /*
    Bit 7    OBJ-to-BG Priority
            0 = Sprite is always rendered above background
            1 = Background colors 1-3 overlay sprite, sprite is still rendered above color 0
    Bit 6    Y-Flip
            If set to 1 the sprite is flipped vertically, otherwise rendered as normal
    Bit 5    X-Flip
            If set to 1 the sprite is flipped horizontally, otherwise rendered as normal
    Bit 4    Palette Number
            If set to 0, the OBP0 register is used as the palette, otherwise OBP1
    Bit 3-0  CGB-Only flags
    */
}oamEntry;

typedef struct _oamLineEntry{
    oamEntry entry;
    struct _oamLineEntry *next;
}oamLineEntry;


typedef struct{
    oamEntry oamRam[40];
    u8 vram[0x2000];
    pixelFifoContext pfc;
    u32 currentFrame;
    u32* vBuffer;
    u8 lineSpriteCount;
    oamLineEntry *lineSprites;
    oamLineEntry lineEntryArray[10];
    u8 fetchedEntryCount;
    oamEntry fetchedEntries[3];
    
    u8 windowLine;

    int tCycles;
    u8 numberOfOp;

    bool enableWindow;

}ppuContext;


ppuContext* getPpuContext();

void initPpu();
void stepPpu();

void writeOam(u16, u8);
u8 readOam(u16);

void writeVram(u16, u8);
u8 readVram(u16);

void processPipeline();
void pipelineFifoBackgroundReset();
void pipelineFifoSpriteReset();

bool windowVisible();