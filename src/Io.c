#include <Io.h>
#include <Utils.h>
#include <Timer.h>
#include <Dma.h>
#include <Lcd.h>
#include <Gamepad.h>
static char serialData[2];


static u8 ly = 0;

u8 readIo(u16 address){
    /*if(address == 0xFF00){
        return 1;
    }*/

    if (address == 0xFF00) {
        return getGamePadOutput();
    }

    if(address == 0xFF01){
        return serialData[0];
    }
    if(address == 0xFF02){
        return serialData[1];
    }
    if(BETWEEN(address, 0xFF04, 0xFF07)){
        return readTimer(address);
    }

    if(BETWEEN(address, 0xFF40, 0xFF4B)){
        return readLcd(address);
    }

    if(BETWEEN(address, 0xFF10, 0xFF3F)){
        return 0;
    }

    if (address == 0xFF0F) {
        return readInterruptFlags();
    }
    printf("\tERR: READ IO: 0x%04X\n", address);
    return 0;
}

void writeIo(u16 address, u8 value){
    if (address == 0xFF00) {
        setSelectGamepad(value);
        return;
    }

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
        printf("wrote to timer 0x%04X %d\n",address, value);
        return;
    }

    if(BETWEEN(address, 0xFF10, 0xFF3F)){
        return;
    }

    if(BETWEEN(address, 0xFF40, 0xFF4B)){
        writeLcd(address, value);
        return;
    }

    if (address == 0xFF0F) {
        setInterruptFlags(value);
        return;
    }

    printf("\tERR: WRT IO: 0x%04X 0x%02X\n", address, value);
    return;
}