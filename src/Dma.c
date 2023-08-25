#include <Dma.h>
#include <Bus.h>
#include <Ppu.h>

static dmaContext DMA = {0};

void startDma(u8 start){
    DMA.active = true;
    DMA.byte = 0;
    DMA.delay = 2;
    DMA.value = start;
}
void stepDma(){
    if(!DMA.active){
        return;
    }
    if(DMA.delay){
        DMA.delay--;
        return;
    }
    writeOam(DMA.byte, readBus(DMA.value * 0x100) + DMA.byte);
    DMA.byte++;
    DMA.active = DMA.byte < 0xA0;
}
bool isTransferingDma(){
    return DMA.active;
}

dmaContext* getDmaContext(){
    return &DMA;
}