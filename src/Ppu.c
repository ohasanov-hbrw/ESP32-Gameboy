#include <Ppu.h>
#include <string.h>
#include <PpuSm.h>
#include <Lcd.h>

static ppuContext PPU = {0};

ppuContext* getPpuContext(){
    return &PPU;
}

void initPpu(){
    PPU.currentFrame = 0;
    PPU.vBuffer = malloc(YRES * XRES * sizeof(32));

    PPU.tCycles = 0;

    PPU.pfc.lineX = 0;
    PPU.pfc.pushedX = 0;
    PPU.pfc.fetchedX = 0;
    PPU.pfc.pixelFifoBackground.size = 0;
    PPU.pfc.pixelFifoBackground.head = PPU.pfc.pixelFifoBackground.tail = NULL;
    PPU.pfc.pixelFifoSprite.size = 0;
    PPU.pfc.pixelFifoSprite.head = PPU.pfc.pixelFifoSprite.tail = NULL;
    PPU.pfc.currentFetchState = FS_TILE;
    PPU.numberOfOp = 0;

    PPU.lineSprites = 0;
    PPU.fetchedEntryCount = 0;
    PPU.windowLine = 0;
    PPU.enableWindow = false;
    PPU.lastTileIndex = -1;

    PPU.scrollX = -1;
    PPU.lastX = -1;
    PPU.scrollY = -1;
    PPU.lastY = -1;

    initLcd();
    LCDS_MODE_SET(MODE_OAM);

    memset(PPU.oamRam, 0, sizeof(PPU.oamRam));
    memset(PPU.vBuffer, 0, YRES * XRES * sizeof(u32));
}

void stepPpu(){
    PPU.tCycles++;
    //printf("LCD LY = 0x%02X tCycles = %d", getLcdContext()->lY, PPU.tCycles);
    switch(LCDS_MODE){
        case MODE_OAM:
            //printf(" MODE_OAM\n");
            oamMode();
            break;
        case MODE_XFER:
            //printf(" MODE_XFER\n");
            xferMode();
            break;
        case MODE_VBLANK:
            //printf(" MODE_VBLANK\n");
            vblankMode();
            break;
        case MODE_HBLANK:
            //printf(" MODE_HBLANK\n");
            hblankMode();
            break;
    }
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

