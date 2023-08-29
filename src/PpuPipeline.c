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

bool pipelineFifoAdd(){

}

void loadSpriteTile(){

}


void loadSpriteData(u8 offset){
    
}

bool pipelinePush(){
    if(getPpuContext()->pfc.pixelFifoBackground.size > 8){
        u32 pixelDataBg = pixelFifoBackgroundPop();
        getPpuContext()->vBuffer[getPpuContext()->pfc.pushedX + (getLcdContext()->lY * XRES)] = pixelDataBg;
        //printf("pushed color 0x%08X\n", pixelDataBg);
        getPpuContext()->pfc.pushedX++;
        return true;
    }
    return false;
}

void loadNextBackgroundPixels(){

}


void pipelineFetch(){
    switch(getPpuContext()->pfc.currentFetchState){
        case FS_TILE:{
            
            if(getPpuContext()->numberOfOp == 0){
                u16 address = 0x9800;
                if(BIT(getLcdContext()->lcdC, 6) && getPpuContext()->enableWindow){
                    address = 0x9C00;
                }
                else if(BIT(getLcdContext()->lcdC, 3) && !getPpuContext()->enableWindow){
                    address = 0x9C00;
                }
                getPpuContext()->x = (getPpuContext()->pfc.pushedX + getPpuContext()->scrollX) / 8;
                getPpuContext()->y = (getLcdContext()->lY + getPpuContext()->scrollY) / 8;
                if(getPpuContext()->enableWindow){
                    getPpuContext()->y = (getPpuContext()->windowLine) / 8;
                    getPpuContext()->x = (getPpuContext()->pfc.pushedX - getLcdContext()->winX - 7) / 8;
                }
                if(getPpuContext()->lastX == getPpuContext()->x){
                    getPpuContext()->x++;
                }
                getPpuContext()->lastX = getPpuContext()->x;
                getPpuContext()->lastY = getPpuContext()->y;

                if(getPpuContext()->x > 32)
                    getPpuContext()->x -= 32;
                if(getPpuContext()->y > 32)
                    getPpuContext()->y -= 32;
                //printf("fetching 8 pixel starting at %d %d  while size was %d...\n", getPpuContext()->x, getPpuContext()->y, getPpuContext()->pfc.pixelFifoBackground.size);
                getPpuContext()->pfc.bgwFetchData[0] = readBus(address + getPpuContext()->x + getPpuContext()->y * 32);
                if(BIT(getLcdContext()->lcdC, 4)){
                    getPpuContext()->pfc.bgwFetchData[0] += 128;
                }
                
            }
            if(getPpuContext()->numberOfOp > 0){
                getPpuContext()->pfc.currentFetchState = FS_DATA0;
                getPpuContext()->numberOfOp = 0;
            }
            getPpuContext()->numberOfOp++;
            break;
        }
        case FS_DATA0:{
            if(getPpuContext()->numberOfOp == 0){
                getPpuContext()->pfc.bgwFetchData[1] = readBus(0x9800 + (getPpuContext()->pfc.bgwFetchData[0] * 16) + getPpuContext()->y * 2);
            }
            if(getPpuContext()->numberOfOp > 0){
                getPpuContext()->pfc.currentFetchState = FS_DATA1;
                getPpuContext()->numberOfOp = 0;
            }
            getPpuContext()->numberOfOp++;
            break;
        } 
        case FS_DATA1:{
            if(getPpuContext()->numberOfOp == 0){
                getPpuContext()->pfc.bgwFetchData[2] = readBus(0x9800 + (getPpuContext()->pfc.bgwFetchData[0] * 16) + getPpuContext()->y * 2 + 1);
            }
            if(getPpuContext()->numberOfOp > 0){
                getPpuContext()->pfc.currentFetchState = FS_IDLE;
                getPpuContext()->numberOfOp = 0;
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
            }
            getPpuContext()->numberOfOp++;
            break;
        }

        case FS_PUSH:{
            if(getPpuContext()->pfc.pixelFifoBackground.size <= 8){
                u8 x = getPpuContext()->pfc.pushedX + getPpuContext()->scrollX;
                //printf("pushing 8 pixel starting at %d %d  while size was %d...", getPpuContext()->pfc.pushedX, getLcdContext()->lY, getPpuContext()->pfc.pixelFifoBackground.size);
                if(getPpuContext()->enableWindow){
                    x = getPpuContext()->pfc.pushedX - getLcdContext()->winX - 7;
                }
                
                for(int i = 0; i < 8; i++){
                    int bit = 7 - i;
                    u8 high = !!(getPpuContext()->pfc.bgwFetchData[1] & (1 << bit));
                    u8 low = !!(getPpuContext()->pfc.bgwFetchData[2] & (1 << bit)) << 1;
                    //printf("tile %d 0x%02X 0x%02X\n", getPpuContext()->pfc.bgwFetchData[0], getPpuContext()->pfc.bgwFetchData[1], getPpuContext()->pfc.bgwFetchData[2]);
                    u32 color = getLcdContext()->bgc[high | low];
                    if(!LCDC_BGW_ENABLE){
                        color = getLcdContext()->bgc[0];
                    }
                    if(LCDC_OBJ_ENABLE){

                    }
                    if (x >= 0) {
                        pixelFifoBackgroundPush(color);
                    }
                    
                }
                if(getPpuContext()->resetScroll){
                    for(int i = 0; i < x % 8; i++){
                        pixelFifoBackgroundPop();
                    }
                    getPpuContext()->resetScroll = false;
                }
                
                getPpuContext()->pfc.currentFetchState = FS_TILE;
                getPpuContext()->numberOfOp = 0;
                //printf(" size after push is %d\n", getPpuContext()->pfc.pixelFifoBackground.size);
            }
            break;
        }
    }
}

void processPipeline(){
    if(getPpuContext()->tCycles == 81){
        pipelineFifoBackgroundReset();
        getPpuContext()->pfc.currentFetchState = FS_TILE;
        getPpuContext()->lastTileIndex = -1;
    }

    if(LCDC_WIN_ENABLE
    && getPpuContext()->pfc.pushedX >= getLcdContext()->winX - 7
    && getPpuContext()->pfc.pushedX < getLcdContext()->winX - 7 + 256
    && getLcdContext()->lY >= getLcdContext()->winY
    && getLcdContext()->lY < getLcdContext()->winY + 256
    && getPpuContext()->enableWindow == false){
        pipelineFifoBackgroundReset();
        getPpuContext()->lastTileIndex = -1;
        getPpuContext()->pfc.currentFetchState = FS_TILE;
        getPpuContext()->enableWindow = true;
        getPpuContext()->windowLine = getLcdContext()->lY - getLcdContext()->winY;
    }
    else{
        if(getPpuContext()->enableWindow == true){
            pipelineFifoBackgroundReset();
            getPpuContext()->pfc.currentFetchState = FS_TILE;
        }
        getPpuContext()->enableWindow = false;
    }
    pipelineFetch();
    pipelinePush();
}

void pipelineFifoBackgroundReset(){
    while(getPpuContext()->pfc.pixelFifoBackground.size) {
        pixelFifoBackgroundPop();
    }
    getPpuContext()->resetScroll = true;
    getPpuContext()->pfc.pixelFifoBackground.head = 0;
}
