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

void pixelFifoSpritePush(u32 value){
    fifoEntry *next = malloc(sizeof(fifoEntry));
    next->next = NULL;
    next->value = value;
    if(!getPpuContext()->pfc.pixelFifoSprite.head){
        getPpuContext()->pfc.pixelFifoSprite.head = getPpuContext()->pfc.pixelFifoSprite.tail = next;
    }
    else{
        getPpuContext()->pfc.pixelFifoSprite.tail->next = next;
        getPpuContext()->pfc.pixelFifoSprite.tail = next;
    }
    getPpuContext()->pfc.pixelFifoSprite.size++;
}

u32 pixelFifoSpritePop(){
    if (getPpuContext()->pfc.pixelFifoSprite.size <= 0) {
        fprintf(stderr, "ERR IN PIXEL FIFO!\n");
        exit(-8);
    }
    fifoEntry *popped = getPpuContext()->pfc.pixelFifoSprite.head;
    getPpuContext()->pfc.pixelFifoSprite.head = popped->next;
    getPpuContext()->pfc.pixelFifoSprite.size--;
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

void pipelineFetch(){
    switch(getPpuContext()->pfc.currentFetchState){
        case FS_TILE:{
            if(getPpuContext()->numberOfOp == 0){

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
        pipelineFifoSpriteReset();
        pipelineFifoBackgroundReset();
        getPpuContext()->pfc.currentFetchState = FS_TILE;
    }

    getPpuContext()->pfc.mapY = (getLcdContext()->lY + getLcdContext()->scrollY);
    getPpuContext()->pfc.mapX = (getPpuContext()->pfc.fetchedX + getLcdContext()->scrollX);
    getPpuContext()->pfc.tileY = ((getLcdContext()->lY + getLcdContext()->scrollY) % 8) * 2;
    u16 xLocation = getPpuContext()->pfc.mapX;
    if(LCDC_WIN_ENABLE
    && getPpuContext()->pfc.pushedX >= getLcdContext()->winX - 7
    && getPpuContext()->pfc.pushedX < getLcdContext()->winX - 7 + 256
    && getLcdContext()->lY >= getLcdContext()->winY
    && getLcdContext()->lY < getLcdContext()->winY + 256){
        pipelineFifoSpriteReset();
        pipelineFifoBackgroundReset();
        getPpuContext()->pfc.currentFetchState = FS_TILE;
        getPpuContext()->enableWindow = true;
        getPpuContext()->windowLine = getLcdContext()->lY - getLcdContext()->winY;
    }
    else{
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
void pipelineFifoSpriteReset(){
    while(getPpuContext()->pfc.pixelFifoSprite.size) {
        pixelFifoSpritePop();
    }
    getPpuContext()->pfc.pixelFifoSprite.head = 0;
}