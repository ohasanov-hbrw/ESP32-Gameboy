#include <Ppu.h>
#include <PpuSm.h>
#include <Lcd.h>
#include <Cpu.h>
#include <Interrupts.h>
#include <string.h>

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

void loadLineSprites(int i){
    int currentLine = getLcdContext()->lY;
    memset(getPpuContext()->lineEntryArray, 0, sizeof(getPpuContext()->lineEntryArray));
    u8 spriteSize = LCDC_OBJ_HEIGHT;
    oamEntry e = getPpuContext()->oamRam[i];
    if(e.x == 0){
        return;
    }
    if(getPpuContext()->lineSpriteCount >= 10){
        return;
    }
    if(e.y - 16 <= currentLine && e.y + spriteSize - 16 > currentLine){
        oamLineEntry *entry = &getPpuContext()->lineEntryArray[getPpuContext()->lineSpriteCount];
        entry->entry = e;
        entry->next = NULL;
        if(!getPpuContext()->lineSprites || getPpuContext()->lineSprites->entry.x > e.x){
            entry->next = getPpuContext()->lineSprites;
            getPpuContext()->lineSprites = entry;
            return;
        }
        oamLineEntry *le = getPpuContext()->lineSprites;
        oamLineEntry *prev = le;
        while(le){
            if(le->entry.x > e.x){
                prev->next = entry;
                entry->next = le;
                break;
            }
            if(!le->next){
                le->next = entry;
                break;
            }
            prev = le;
            le = le->next;
        }
    }
}

static u32 target_frame_time = 1000 / 60;
static long prev_frame_time = 0;
static long start_timer = 0;
static long frame_count = 0;

void oamMode(){
    if(getLcdContext()->lY >= YRES){
        LCDS_MODE_SET(MODE_VBLANK);
        vblankMode();
        return;
    }
    if(getPpuContext()->tCycles > 80){
        LCDS_MODE_SET(MODE_XFER);
        getPpuContext()->pfc.currentFetchState = FS_TILE;
        getPpuContext()->pfc.lineX = 0;
        getPpuContext()->pfc.fetchedX = 0;
        getPpuContext()->pfc.pushedX = 0;
        getPpuContext()->pfc.fifoX = 0;
        getPpuContext()->lastX = -1;
        getPpuContext()->lastY = -1;
        getPpuContext()->scrollX = getLcdContext()->scrollX;
        getPpuContext()->scrollY = getLcdContext()->scrollY;
        getPpuContext()->resetScroll = true;
        getPpuContext()->lastTileIndex = -1;
        getPpuContext()->enableWindow = false;
        xferMode();
        return;
    }
    if(getPpuContext()->tCycles == 1){
        getPpuContext()->lineSprites = 0;
        getPpuContext()->lineSpriteCount = 0;
        getPpuContext()->currentIndexOfSprite = 0;
    }
    if(getPpuContext()->tCycles % 2 == 0){
        loadLineSprites(getPpuContext()->tCycles / 2);
    }
}

void xferMode(){
    processPipeline();
    if(getPpuContext()->pfc.pushedX >= XRES || getPpuContext()->tCycles > 400){
        pipelineFifoBackgroundReset();
        LCDS_MODE_SET(MODE_HBLANK);
        if(LCDS_STAT_INT(SS_HBLANK)){
            requestInterrupt(IT_LCD_STAT);
        }
    }
}

void vblankMode(){
    if (getPpuContext()->tCycles >= TICKS_PER_LINE){
        incrementLy();
        if (getLcdContext()->lY >= LINES_PER_FRAME){
            LCDS_MODE_SET(MODE_OAM);
            getLcdContext()->lY = 0;
            getPpuContext()->windowLine = 0;
        }
        getPpuContext()->tCycles = 0;
    }
}

void hblankMode(){
    if (getPpuContext()->tCycles >= TICKS_PER_LINE) {
        incrementLy();
        if (getLcdContext()->lY >= YRES) {
            LCDS_MODE_SET(MODE_VBLANK);
            requestInterrupt(IT_VBLANK);
            if (LCDS_STAT_INT(SS_VBLANK)) {
                requestInterrupt(IT_LCD_STAT);
            }
            getPpuContext()->currentFrame++;

            //calc FPS...
            u32 end = getTicks();
            u32 frame_time = end - prev_frame_time;

            if (frame_time < target_frame_time) {
                delay((target_frame_time - frame_time));
            }

            if (end - start_timer >= 1000) {
                u32 fps = frame_count;
                start_timer = end;
                frame_count = 0;

                printf("FPS: %d\n", fps);
            }

            frame_count++;
            prev_frame_time = getTicks();

        }
        else{
            LCDS_MODE_SET(MODE_OAM);
        }
        getPpuContext()->tCycles = 0;
    }
}