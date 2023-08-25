#include <Io.h>
#include <Utils.h>
#include <Timer.h>
#include <Dma.h>
static char serialData[2];


static u8 ly = 0;

u8 readIo(u16 address){
    if(address == 0xFF01){
        return serialData[0];
    }
    if(address == 0xFF02){
        return serialData[1];
    }
    if(BETWEEN(address, 0xFF04, 0xFF07)){
        return readTimer(address);
    }
    
    if (address == 0xFF44) {
        return ly++;
    }

    if (address == 0xFF0F) {
        return readInterruptFlags();
    }
    printf("\tERR: READ IO: 0x%04X\n", address);
    return 0;
}

void writeIo(u16 address, u8 value){
    if(address == 0xFF01){
        serialData[0] = value;
        return;
    }

    if(address == 0xFF02){
        serialData[1] = value;
        return;
    }

    if(BETWEEN(address, 0xFF04, 0xFF07)){
        writeTimer(address, value);
        return;
    }

    if (address == 0xFF46) {
        startDma(value);
    }

    if (address == 0xFF0F) {
        setInterruptFlags(value);
        return;
    }

    printf("\tERR: WRT IO: 0x%04X 0x%02X\n", address, value);
    return;
}