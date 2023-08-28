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
        if(getPpuContext()->pfc.lineX >= (getLcdContext()->scrollX % 8)){
            getPpuContext()->vBuffer[getPpuContext()->pfc.pushedX + (getLcdContext()->lY * XRES)] = pixelDataBg;
            getPpuContext()->pfc.pushedX++;
        }
        getPpuContext()->pfc.lineX++;
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

                /*getPpuContext()->pfc.mapY = (getLcdContext()->lY + getLcdContext()->scrollY);
                getPpuContext()->pfc.mapX = (getPpuContext()->pfc.fetchedX + getLcdContext()->scrollX);
                getPpuContext()->pfc.tileY = ((getLcdContext()->lY + getLcdContext()->scrollY) % 8) * 2;
                u16 xLocation = getPpuContext()->pfc.mapX;*/
                u16 address = 0x9800;
                if(BIT(getLcdContext()->lcdC, 6) && getPpuContext()->enableWindow){
                    address = 0x9C00;
                }
                else if(BIT(getLcdContext()->lcdC, 3) && !getPpuContext()->enableWindow){
                    address = 0x9C00;
                }
                u8 x = getPpuContext()->pfc.pushedX / 8;
                u8 y = (getLcdContext()->lY + getLcdContext()->scrollY) / 8;
                if(getPpuContext()->enableWindow){
                    y = (getPpuContext()->windowLine) / 8;
                    x = (getPpuContext()->pfc.pushedX - getLcdContext()->winX - 7) / 8;
                }
                if(getPpuContext()->lastTileIndex == x){
                    return;
                }
                else{
                    getPpuContext()->lastTileIndex = x;
                }

                getPpuContext()->pfc.bgwFetchData[0] = readBus(address + x + y * 32);
                
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
            if(getPpuContext()->pfc.pixelFifoBackground.size > 8){
                break;
            }
            getPpuContext()->pfc.currentFetchState = FS_TILE;
            getPpuContext()->numberOfOp = 0;
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
    getPpuContext()->pfc.pixelFifoBackground.head = 0;
}
