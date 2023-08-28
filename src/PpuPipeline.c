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

void pipelinePush(){
    if(getPpuContext()->pfc.pixelFifoBackground.size > 8){
        u32 pixelDataBg = pixelFifoBackgroundPop();
        if(getPpuContext()->pfc.lineX >= (getLcdContext()->scrollX % 8)){
            getPpuContext()->vBuffer[getPpuContext()->pfc.pushedX + (getLcdContext()->lY * XRES)] = pixelDataBg;
            getPpuContext()->pfc.pushedX++;
        }
        getPpuContext()->pfc.lineX++;
    }
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
    getPpuContext()->pfc.mapY = (getLcdContext()->lY + getLcdContext()->scrollY);
    getPpuContext()->pfc.mapX = (getPpuContext()->pfc.fetchedX + getLcdContext()->scrollX);
    getPpuContext()->pfc.tileY = ((getLcdContext()->lY + getLcdContext()->scrollY) % 8) * 2;
    if(getPpuContext()->tCycles == 81){
        //empty fifo
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