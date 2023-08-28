#include <Ppu.h>
#include <PpuSm.h>
#include <Lcd.h>
#include <Cpu.h>
#include <Interrupts.h>
#include <string.h>

void incrementLy(){ 

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
        xferMode();
        return;
    }
    if(getPpuContext()->tCycles == 1){
        getPpuContext()->lineSprites = 0;
        getPpuContext()->lineSpriteCount = 0;
    }
    if(getPpuContext()->tCycles % 2 == 0){
        loadLineSprites(getPpuContext()->tCycles / 2);
    }
}

void xferMode(){

}

void vblankMode(){

}

void hblankMode(){

}