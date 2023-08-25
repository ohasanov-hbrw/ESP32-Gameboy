#include <Ppu.h>
#include <Lcd.h>

void pixelFifoPush(u32 value){
    fifoEntry *next = malloc(sizeof(fifoEntry));
    next->next = NULL;
    next->value = value;

    if(!getPpuContext()->pfc.pixelFifo.head){
        getPpuContext()->pfc.pixelFifo.head = getPpuContext()->pfc.pixelFifo.tail = next;
    }
    else{
        getPpuContext()->pfc.pixelFifo.tail->next = next;
        getPpuContext()->pfc.pixelFifo.tail = next;
    }

    getPpuContext()->pfc.pixelFifo.size++;
}

u32 pixelFifoPop(){
    if(getPpuContext()->pfc.pixelFifo.size <= 0){
        printf("\tERR: INV PIXEL FIFO\n");
        exit(-4);
    }

    fifoEntry *popped = getPpuContext()->pfc.pixelFifo.head;
    getPpuContext()->pfc.pixelFifo.head = popped->next;
    getPpuContext()->pfc.pixelFifo.size--;

    u32 value = popped->value;
    free(popped);
    return value;
}

u32 fetchSpritePixels(int bit, u32 color, u8 bgColor){
    for(int i = 0; i < getPpuContext()->fetchedEntryCount; i++){
        int spX = (getPpuContext()->fetchedEntries[i].x - 8) + (getLcdContext()->scrollX % 8);

        if(spX + 8 < getPpuContext()->pfc.fifoX){
            continue;
        }
        int offset = getPpuContext()->pfc.fifoX - spX;
        if(offset < 0 || offset > 7){
            continue;
        }
        bit = (7 - offset);
        if(getPpuContext()->fetchedEntries[i].fxflip){
            bit = offset;
        }
        u8 high = !!(getPpuContext()->pfc.entryFetchData[i * 2] & (1 << bit));
        u8 low = !!(getPpuContext()->pfc.entryFetchData[(i * 2) + 1] & (1 << bit)) << 1;
        bool backgroundPriority = getPpuContext()->fetchedEntries[i].fbgp;
        if(!(high | low)){
            continue;
        }
        if(!backgroundPriority || bgColor == 0){
            color = (getPpuContext()->fetchedEntries[i].fpn) ? getLcdContext()->spc2[high | low] : getLcdContext()->spc1[high | low];
            if(high | low){
                break;
            }
        }
    }
    return color;
}

bool pipelineFifoAdd(){
    if(getPpuContext()->pfc.pixelFifo.size > 8){
        return false;
    }
    int x = getPpuContext()->pfc.fetchedX - (8 - (getLcdContext()->scrollX % 8));
    for(int i = 0; i < 8; i++){
        int bit = 7 - i;
        u8 high = !!(getPpuContext()->pfc.bgwFetchData[1] & (1 << bit));
        u8 low = !!(getPpuContext()->pfc.bgwFetchData[2] & (1 << bit)) << 1;
        u32 color = getLcdContext()->bgc[high | low];

        if(!LCDC_BGW_ENABLE){
            color = getLcdContext()->bgc[0];
        }
        if(LCDC_OBJ_ENABLE){
            color = fetchSpritePixels(bit, color, high | low);
        }

        if(x >= 0){
            pixelFifoPush(color);
            getPpuContext()->pfc.fifoX++;
        }
    }
    return true;
}


void loadSpriteTile(){
    oamLineEntry *line = getPpuContext()->lineSprites;
    while(line){
        int spX = (line->entry.x - 8) + (getLcdContext()->scrollX % 8);
        if((spX >= getPpuContext()->pfc.fetchedX && spX < getPpuContext()->pfc.fetchedX + 8) || (spX + 8 >= getPpuContext()->pfc.fetchedX && spX + 8 < getPpuContext()->pfc.fetchedX + 8)){
            getPpuContext()->fetchedEntries[getPpuContext()->fetchedEntryCount++] = line->entry;
        }
        line = line->next;
        if(!line || getPpuContext()->fetchedEntryCount >= 3){
            break;
        }
    }
}


void loadSpriteData(u8 offset){
    int currentY = getLcdContext()->lY;
    u8 spriteHeight = LCDC_OBJ_HEIGHT;
    for(int i = 0; i < getPpuContext()->fetchedEntryCount; i++){
        u8 tileY = ((currentY + 16) - getPpuContext()->fetchedEntries[i].y) * 2;
        if(getPpuContext()->fetchedEntries[i].fyflip){
            tileY = ((spriteHeight * 2) - 2) - tileY;
        }
        u8 tileIndex = getPpuContext()->fetchedEntries[i].tile;
        if(spriteHeight == 16){
            tileIndex &= ~(1);
        }
        getPpuContext()->pfc.entryFetchData[(i * 2) + offset] = readBus(0x8000 + (tileIndex * 16) + tileY + offset);
    }
}

void pipelineFetch(){
    switch(getPpuContext()->pfc.currentFetchState){
        case FS_TILE:{
            getPpuContext()->fetchedEntryCount = 0;
            if(LCDC_BGW_ENABLE){
                getPpuContext()->pfc.bgwFetchData[0] = readBus(LCDC_BG_MAP_AREA + (getPpuContext()->pfc.mapX / 8) + (((getPpuContext()->pfc.mapY / 8)) * 32));
            
                if(LCDC_BGW_DATA_AREA == 0x8800){
                    getPpuContext()->pfc.bgwFetchData[0] += 128;
                }
            }

            if(LCDC_OBJ_ENABLE && getPpuContext()->lineSprites){
                loadSpriteTile();
            }

            getPpuContext()->pfc.currentFetchState = FS_DATA0;
            getPpuContext()->pfc.fetchedX += 8;
            break;
        }
        case FS_DATA0:{
            getPpuContext()->pfc.bgwFetchData[1] = readBus(LCDC_BGW_DATA_AREA +
               (getPpuContext()->pfc.bgwFetchData[0] * 16) + 
                getPpuContext()->pfc.tileY);
            loadSpriteData(0);
            getPpuContext()->pfc.currentFetchState = FS_DATA1;
            break;
        }
        case FS_DATA1:{
            getPpuContext()->pfc.bgwFetchData[2] = readBus(LCDC_BGW_DATA_AREA + (getPpuContext()->pfc.bgwFetchData[0] * 16) +  getPpuContext()->pfc.tileY + 1);
            loadSpriteData(1);
            getPpuContext()->pfc.currentFetchState = FS_IDLE;
            break;
        }
        case FS_IDLE:{
            getPpuContext()->pfc.currentFetchState = FS_PUSH;
            break;
        } 
        case FS_PUSH:{
            if(pipelineFifoAdd()){
                getPpuContext()->pfc.currentFetchState = FS_TILE;
            }
            break;
        } 
    }
}

void pipelinePush(){
    if(getPpuContext()->pfc.pixelFifo.size > 8){
        u32 pixelData = pixelFifoPop();
        if(getPpuContext()->pfc.lineX >= (getLcdContext()->scrollX % 8)){
            getPpuContext()->vBuffer[getPpuContext()->pfc.pushedX + (getLcdContext()->lY * XRES)] = pixelData;
            getPpuContext()->pfc.pushedX++;
        }
        getPpuContext()->pfc.lineX++;
    }
}

void processPipeline(){
    getPpuContext()->pfc.mapY = (getLcdContext()->lY + getLcdContext()->scrollY);
    getPpuContext()->pfc.mapX = (getPpuContext()->pfc.fetchedX + getLcdContext()->scrollX);
    getPpuContext()->pfc.tileY = ((getLcdContext()->lY + getLcdContext()->scrollY) % 8) * 2; 
    if(!(getPpuContext()->lineTicks & 1)){
        pipelineFetch();
    }
    pipelinePush();
}

void pipelineFifoReset(){
    while(getPpuContext()->pfc.pixelFifo.size){
        pixelFifoPop();
    }
    getPpuContext()->pfc.pixelFifo.head = 0;
}