#include <Ppu.h>
#include <PpuSm.h>
#include <Lcd.h>
#include <Cpu.h>
#include <Interrupts.h>

void incrementLy(){
    getLcdContext()->lY++;

    if(getLcdContext()->lY == getLcdContext()->lYCompare){
        LCDS_LYC_SET(1);
        if(LCDS_STAT_INT(SS_LYC)){
            requestInterrupt(IT_LCD_STAT);
        }
    }
    else{
        LCDS_LYC_SET(0);
    }
}

void oamMode(){
    if(getPpuContext()->lineTicks >= 80){
        LCDS_MODE_SET(MODE_XFER);
        getPpuContext()->pfc.currentFetchState = FS_TILE;
        getPpuContext()->pfc.lineX = 0;
        getPpuContext()->pfc.fetchedX = 0;
        getPpuContext()->pfc.pushedX = 0;
        getPpuContext()->pfc.fifoX = 0;
    }
}

void xferMode(){
    processPipeline();
    if(getPpuContext()->pfc.pushedX >= XRES){
        pipelineFifoReset();
        LCDS_MODE_SET(MODE_HBLANK);
        if(LCDS_STAT_INT(SS_VBLANK)){
            requestInterrupt(IT_LCD_STAT);
        }
    }
}

void vblankMode(){
    if(getPpuContext()->lineTicks >= TICKS_PER_LINE){
        incrementLy();
        if (getLcdContext()->lY >= LINES_PER_FRAME){
            LCDS_MODE_SET(MODE_OAM);
            getLcdContext()->lY = 0;
        }
        getPpuContext()->lineTicks = 0;
    }
}

static u32 targetFrameTime = 1000 / 60;
static long previousFrameTime = 0;
static long timerStart = 0;
static long frameCount = 0;

void hblankMode(){
    if(getPpuContext()->lineTicks >= TICKS_PER_LINE){
        incrementLy();
        if(getLcdContext()->lY >= YRES){
            LCDS_MODE_SET(MODE_VBLANK);
            requestInterrupt(IT_VBLANK);
            if(LCDS_STAT_INT(SS_VBLANK)){
                requestInterrupt(IT_LCD_STAT);
            }
            getPpuContext()->currentFrame++;
            u32 end = getTicks();
            u32 frameTime = end - previousFrameTime;
            if (frameTime < targetFrameTime) {
                delay((targetFrameTime - frameTime));
            }
            if (end - timerStart >= 1000) {
                u32 fps = frameCount;
                timerStart = end;
                frameCount = 0;

                printf("FPS: %d\n", fps);
            }
            frameCount++;
            previousFrameTime = getTicks();

        }
        else{
            LCDS_MODE_SET(MODE_OAM);
        }
        getPpuContext()->lineTicks = 0;
    }
}