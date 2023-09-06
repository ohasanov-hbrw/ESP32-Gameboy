#include <Ppu.h>
#include <Lcd.h>


bool windowVisible(){
    
}


void pixelFifoBackgroundPush(u32 value){
    fifoEntry *next = malloc(sizeof(fifoEntry));
    next->next = NULL;
    next->value = value;
    if(!getPpuContext()->pfc.pixelFifoBackground.head){
        getPpuContext()->pfc.pixelFifoBackground.head = getPpuContext()->pfc.pixelFifoBackground.tail = next;
    }
    else{
        getPpuContext()->pfc.pixelFifoBackground.tail->next = next;
        getPpuContext()->pfc.pixelFifoBackground.tail = next;
    }
    getPpuContext()->pfc.pixelFifoBackground.size++;
}

u32 pixelFifoBackgroundPop(){
    if (getPpuContext()->pfc.pixelFifoBackground.size <= 0) {
        fprintf(stderr, "ERR IN PIXEL FIFO!\n");
        exit(-8);
    }
    fifoEntry *popped = getPpuContext()->pfc.pixelFifoBackground.head;
    getPpuContext()->pfc.pixelFifoBackground.head = popped->next;
    getPpuContext()->pfc.pixelFifoBackground.size--;
    u32 val = popped->value;
    free(popped);
    return val;
}

u32 fetchSpritePixels(int bit, u32 color, u8 bgColor){

}

bool fifoAdd(){
    if(getPpuContext()->pfc.pixelFifoBackground.size > 8){
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
            pixelFifoBackgroundPush(color);
            getPpuContext()->pfc.fifoX++;
        }
    }

    return true;
}

void loadSpriteTile(){
    oamLineEntry *le = getPpuContext()->lineSprites;
    while(le){
        int sp_x = (le->entry.x - 8) + (getLcdContext()->scrollX % 8);
        if((sp_x >= getPpuContext()->pfc.fetchedX && sp_x < getPpuContext()->pfc.fetchedX + 8) || ((sp_x + 8) >= getPpuContext()->pfc.fetchedX && (sp_x + 8) < getPpuContext()->pfc.fetchedX + 8)) {
            getPpuContext()->fetchedEntries[getPpuContext()->fetchedEntryCount++] = le->entry;
        }
        le = le->next;
        if(!le || getPpuContext()->fetchedEntryCount >= 3){
            break;
        }
    }
}

void loadWindowTile(){

}

void loadSpriteData(u8 offset){
    
}

bool pipelinePush(){
    if(getPpuContext()->pfc.pixelFifoBackground.size > 8){
        u32 pixelDataBg = pixelFifoBackgroundPop();
        if(getPpuContext()->pfc.lineX >= (getLcdContext()->scrollX % 8)){
            getPpuContext()->vBuffer[getPpuContext()->pfc.pushedX + (getLcdContext()->lY * XRES)] = pixelDataBg;
            //printf("pushed color 0x%08X\n", pixelDataBg);
            getPpuContext()->pfc.pushedX++;
        }
        getPpuContext()->pfc.lineX++;
        return true;
    }
    return false;
}

void pipelineFetch(){
    switch(getPpuContext()->pfc.currentFetchState){
        case FS_TILE:{
            if(getPpuContext()->numberOfOp == 0){
                getPpuContext()->fetchedEntryCount = 0;
                u16 address = 0x9800;
                /*if(BIT(getLcdContext()->lcdC, 6) && getPpuContext()->enableWindow){
                    address = 0x9C00;
                }
                else if(BIT(getLcdContext()->lcdC, 3) && !getPpuContext()->enableWindow){
                    address = 0x9C00;
                }*/
                if(LCDC_BGW_ENABLE){
                    getPpuContext()->pfc.bgwFetchData[0] = readBus(address + (getPpuContext()->pfc.mapX / 8) + (((getPpuContext()->pfc.mapY / 8)) * 32));
                    if(LCDC_BGW_DATA_AREA == 0x8800) {
                        getPpuContext()->pfc.bgwFetchData[0] += 128;
                    }
                    loadWindowTile();
                }
                if(LCDC_OBJ_ENABLE && getPpuContext()->lineSprites){
                    loadSpriteTile();
                } 
                getPpuContext()->pfc.fetchedX += 8;
            }
            if(getPpuContext()->numberOfOp > 0){
                getPpuContext()->pfc.currentFetchState = FS_DATA0;
                getPpuContext()->numberOfOp = 0;
                break;
            }
            getPpuContext()->numberOfOp++;
            break;
        }
        case FS_DATA0:{
            if(getPpuContext()->numberOfOp == 0){
                getPpuContext()->pfc.bgwFetchData[1] = readBus(LCDC_BGW_DATA_AREA + (getPpuContext()->pfc.bgwFetchData[0] * 16) +  getPpuContext()->pfc.tileY);
                loadSpriteData(0);
            }
            if(getPpuContext()->numberOfOp > 0){
                getPpuContext()->pfc.currentFetchState = FS_DATA1;
                getPpuContext()->numberOfOp = 0;
                break;
            }
            getPpuContext()->numberOfOp++;
            break;
        } 
        case FS_DATA1:{
            if(getPpuContext()->numberOfOp == 0){
                getPpuContext()->pfc.bgwFetchData[2] = readBus(LCDC_BGW_DATA_AREA + (getPpuContext()->pfc.bgwFetchData[0] * 16) +  getPpuContext()->pfc.tileY + 1);
                loadSpriteData(1);
            }
            if(getPpuContext()->numberOfOp > 0){
                getPpuContext()->pfc.currentFetchState = FS_IDLE;
                getPpuContext()->numberOfOp = 0;
                break;
            }
            getPpuContext()->numberOfOp++;
            break;
        }
        case FS_IDLE:{
            if(getPpuContext()->numberOfOp == 0){

            }
            if(getPpuContext()->numberOfOp > 0){
                getPpuContext()->pfc.currentFetchState = FS_PUSH;
                getPpuContext()->numberOfOp = 0;
                break;
            }
            getPpuContext()->numberOfOp++;
            break;
        }

        case FS_PUSH:{
            if(getPpuContext()->numberOfOp % 2 == 0){
                if(fifoAdd()){
                    getPpuContext()->pfc.currentFetchState = FS_TILE;
                    getPpuContext()->numberOfOp = 0;
                    break;
                }
            }
            getPpuContext()->numberOfOp++;
            break;
        }
    }
}

void fakeFetch(){
    switch(getPpuContext()->pfc.currentFetchState){
        case FS_TILE:{
            getPpuContext()->fetchedEntryCount = 0;
            if(getPpuContext()->numberOfOp == 0){

            }
            if(getPpuContext()->numberOfOp > 0){
                getPpuContext()->pfc.currentFetchState = FS_DATA0;
                getPpuContext()->numberOfOp = 0;
                break;
            }
            getPpuContext()->numberOfOp++;
            break;
        }
        case FS_DATA0:{
            if(getPpuContext()->numberOfOp == 0){

            }
            if(getPpuContext()->numberOfOp > 0){
                getPpuContext()->pfc.currentFetchState = FS_DATA1;
                getPpuContext()->numberOfOp = 0;
                break;
            }
            getPpuContext()->numberOfOp++;
            break;
        } 
        case FS_DATA1:{
            if(getPpuContext()->numberOfOp == 0){
 
            }
            if(getPpuContext()->numberOfOp > 0){
                getPpuContext()->pfc.currentFetchState = FS_IDLE;
                getPpuContext()->numberOfOp = 0;
                break;
            }
            getPpuContext()->numberOfOp++;
            break;
        }
        case FS_IDLE:{
            if(getPpuContext()->numberOfOp == 0){

            }
            if(getPpuContext()->numberOfOp > 0){
                getPpuContext()->pfc.currentFetchState = FS_PUSH;
                getPpuContext()->numberOfOp = 0;
                break;
            }
            getPpuContext()->numberOfOp++;
            break;
        }

        case FS_PUSH:{
            if(getPpuContext()->numberOfOp % 2 == 0){
                if(getPpuContext()->fetchedFakePixels > 8){

                }
                else{
                    getPpuContext()->pfc.currentFetchState = FS_TILE;
                    getPpuContext()->numberOfOp = -1;
                    getPpuContext()->fetchedFakePixels += 8;
                    break;
                }
            }
            getPpuContext()->numberOfOp++;
            break;
        }
    }
}

void fakePush(){
    u32 pixelDataBg = getLcdContext()->bgc[0];

    int x = getPpuContext()->pushedFakePixels;
    int y = getLcdContext()->lY;

    int windowY = getLcdContext()->winY;
    int windowX = getLcdContext()->winX - 7;


    if(x >= windowX && x < windowX + 256 && getPpuContext()->windowLatch && LCDC_WIN_ENABLE){
        int mapX = (x - windowX) % 256;
        int mapY = getPpuContext()->windowLine % 256;
        //printf("mapY = %d\n", mapY);

        u16 address = 0x9800;
        if(BIT(getLcdContext()->lcdC, 6)){
            address = 0x9C00;
        }
        if(LCDC_BGW_ENABLE){
            getPpuContext()->pfc.bgwFetchData[0] = readBus(LCDC_WIN_MAP_AREA + (mapX / 8) + (((mapY / 8)) * 32));
            if(LCDC_BGW_DATA_AREA == 0x8800) {
                getPpuContext()->pfc.bgwFetchData[0] += 128;
            }

            getPpuContext()->pfc.bgwFetchData[1] = readBus(LCDC_BGW_DATA_AREA + (getPpuContext()->pfc.bgwFetchData[0] * 16) + (mapY % 8) * 2);
            getPpuContext()->pfc.bgwFetchData[2] = readBus(LCDC_BGW_DATA_AREA + (getPpuContext()->pfc.bgwFetchData[0] * 16) + (mapY % 8) * 2 + 1);
            
            int bit = 7 - (mapX % 8);
            u8 high = !!(getPpuContext()->pfc.bgwFetchData[1] & (1 << bit));
            u8 low = !!(getPpuContext()->pfc.bgwFetchData[2] & (1 << bit)) << 1;
            pixelDataBg = getLcdContext()->bgc[high | low]/* & 0xFFFF00FF*/; //getLcdContext()->bgc[high | low] | 123 
            //printf("stuff on window on line %d \n", y);
        }
        else{
            pixelDataBg = 0xFFFF00FF; //getLcdContext()->bgc[0]
        }
        getPpuContext()->wasInWindow = true;
    }
    else{
        int mapX = (x + getLcdContext()->scrollX) % 256;
        int mapY = (y + getLcdContext()->scrollY) % 256;

        u16 address = 0x9800;
        if(BIT(getLcdContext()->lcdC, 3)){
            address = 0x9C00;
        }
        if(LCDC_BGW_ENABLE){
            getPpuContext()->pfc.bgwFetchData[0] = readBus(LCDC_BG_MAP_AREA + (mapX / 8) + (((mapY / 8)) * 32));
            if(LCDC_BGW_DATA_AREA == 0x8800) {
                getPpuContext()->pfc.bgwFetchData[0] += 128;
            }

            getPpuContext()->pfc.bgwFetchData[1] = readBus(LCDC_BGW_DATA_AREA + (getPpuContext()->pfc.bgwFetchData[0] * 16) +  (mapY % 8) * 2);
            getPpuContext()->pfc.bgwFetchData[2] = readBus(LCDC_BGW_DATA_AREA + (getPpuContext()->pfc.bgwFetchData[0] * 16) +  (mapY % 8) * 2 + 1);
            
            int bit = 7 - (mapX % 8);
            u8 high = !!(getPpuContext()->pfc.bgwFetchData[1] & (1 << bit));
            u8 low = !!(getPpuContext()->pfc.bgwFetchData[2] & (1 << bit)) << 1;
            pixelDataBg = getLcdContext()->bgc[high | low];
        }
        else{
            pixelDataBg = getLcdContext()->bgc[0]; //getLcdContext()->bgc[0]
        }
    }

    oamLineEntry *le = getPpuContext()->lineSprites;
    getPpuContext()->fetchedEntryCount = 0;
    while(le){
        int spriteX = (le->entry.x - 8) + (getLcdContext()->scrollX % 8);
        if((spriteX >= x && spriteX < x + 8) || ((spriteX + 8) >= x  && (spriteX + 8) < x + 8)){
            getPpuContext()->fetchedEntries[getPpuContext()->fetchedEntryCount++] = le->entry;
        }
        le = le->next;
        if(!le || getPpuContext()->fetchedEntryCount >= 3){
            break;
        }
    }

    u8 spriteHeight = LCDC_OBJ_HEIGHT;
    
    for(int offset = 0; offset < 2; offset++){
        for(int i = 0; i < getPpuContext()->fetchedEntryCount; i++){
            u8 ty = ((y + 16) - getPpuContext()->fetchedEntries[i].y) * 2;
            if(BIT(getPpuContext()->fetchedEntries[i].flags, 6)){
                ty = ((spriteHeight * 2) - 2) - ty;
            }
            u8 tileIndex = getPpuContext()->fetchedEntries[i].tile;
            if(spriteHeight == 16){
                tileIndex &= ~(1); //remove last bit...
            }
            getPpuContext()->pfc.entryFetchData[(i * 2) + offset] = readBus(0x8000 + (tileIndex * 16) + ty + offset);
        }
    }

    if(LCDC_OBJ_ENABLE){
        for(int i = 0; i < getPpuContext()->fetchedEntryCount; i++){
            int spriteX = (getPpuContext()->fetchedEntries[i].x - 8) + ((getLcdContext()->scrollX % 8));
            if (spriteX + 8 < getPpuContext()->pushedFakePixels) {
                continue;
            }
            int offset = getPpuContext()->pushedFakePixels - spriteX;
            if (offset < 0 || offset > 7) {
                continue;
            }
            int bit = (7 - offset);
            if(BIT(getPpuContext()->fetchedEntries[i].flags, 5)){
                bit = offset;
            }
            u8 high = !!(getPpuContext()->pfc.entryFetchData[i * 2] & (1 << bit));
            u8 low = !!(getPpuContext()->pfc.entryFetchData[(i * 2) + 1] & (1 << bit)) << 1;

            bool bgp = BIT(getPpuContext()->fetchedEntries[i].flags, 7);

            if(!(high | low)){
                continue;
            }

            if (!bgp || pixelDataBg == getLcdContext()->bgc[0]) {
                pixelDataBg = (BIT(getPpuContext()->fetchedEntries[i].flags, 4)) ? getLcdContext()->spc2[high | low] : getLcdContext()->spc1[high | low];
                if(high | low){
                    break;
                }
            }
        }
    }

    getPpuContext()->vBuffer[getPpuContext()->pushedFakePixels + (getLcdContext()->lY * XRES)] = pixelDataBg;
    getPpuContext()->pushedFakePixels++;
    getPpuContext()->fetchedFakePixels--;
}


void processPipeline(){
    /*getPpuContext()->pfc.mapY = (getLcdContext()->lY + getLcdContext()->scrollY);
    getPpuContext()->pfc.mapX = (getPpuContext()->pfc.fetchedX + getLcdContext()->scrollX);
    getPpuContext()->pfc.tileY = ((getLcdContext()->lY + getLcdContext()->scrollY) % 8) * 2;
    pipelineFetch();
    pipelinePush();*/
    int x = getPpuContext()->pushedFakePixels;
    int y = getLcdContext()->lY;

    int windowY = getLcdContext()->winY;
    int windowX = getLcdContext()->winX - 7;
    if(getPpuContext()->inWindow == false && x >= windowX && x < windowX + 256 && y > windowY && y <= windowY + 256 && LCDC_WIN_ENABLE){
        getPpuContext()->pfc.currentFetchState = FS_TILE;
        getPpuContext()->fetchedFakePixels = 0;
        getPpuContext()->inWindow = true;
    }
    if(getPpuContext()->inWindow == true && !(x >= windowX && x < windowX + 256 && y > windowY && y <= windowY + 256 && LCDC_WIN_ENABLE)){
        getPpuContext()->pfc.currentFetchState = FS_TILE;
        getPpuContext()->fetchedFakePixels = 0;
        getPpuContext()->inWindow = false;
    }

    fakeFetch();
    if(getPpuContext()->pushedFakePixels == 0 && getPpuContext()->fetchedFakePixels > 8){
        getPpuContext()->fetchedFakePixels -= getLcdContext()->scrollX % 8;
    }
    if(getPpuContext()->fetchedFakePixels > 8){
        fakePush();
        //if(getPpuContext()->tCycles - 80 > 172) printf("Line: %d  Cycles: %d  Pushed: %d  FIFO size after pushing: %d\n", y, getPpuContext()->tCycles - 80, getPpuContext()->pushedFakePixels, getPpuContext()->fetchedFakePixels);
    }
}

void pipelineFifoBackgroundReset(){
    while(getPpuContext()->pfc.pixelFifoBackground.size) {
        pixelFifoBackgroundPop();
    }
    //getPpuContext()->resetScroll = true;
    getPpuContext()->pfc.pixelFifoBackground.head = 0;
}
