#include <Bus.h>
#include <GameCartridge.h>
#include <GBRam.h>
#include <Io.h>
#include <Ppu.h>
#include <Dma.h>

// 0x0000 - 0x3FFF : ROM Bank 0
// 0x4000 - 0x7FFF : ROM Bank 1 - Switchable
// 0x8000 - 0x97FF : CHR RAM
// 0x9800 - 0x9BFF : BG Map 1
// 0x9C00 - 0x9FFF : BG Map 2
// 0xA000 - 0xBFFF : Cartridge RAM
// 0xC000 - 0xCFFF : RAM Bank 0
// 0xD000 - 0xDFFF : RAM Bank 1-7 - switchable - Color only
// 0xE000 - 0xFDFF : Reserved - Echo RAM
// 0xFE00 - 0xFE9F : Object Attribute Memory
// 0xFEA0 - 0xFEFF : Reserved - Unusable
// 0xFF00 - 0xFF7F : I/O Registers
// 0xFF80 - 0xFFFE : Zero Page

 

u8 readBus(u16 address){
    if(address < 0x8000){
        return readFromCartridge(address);
    }
    else if(address < 0xA000){
        return readVram(address);
    }
    else if(address < 0xC000){
        return readFromCartridge(address);
    }
    else if(address < 0xE000){
        return readFromWram(address);
    }
    else if(address < 0xFE00){
        return 0;
    }
    else if(address < 0xFEA0){
        return readOam(address);
    }
    else if(address < 0xFF00){
        return 0;
    }
    else if(address < 0xFF80){
        return readIo(address);
    }
    else if(address == 0xFFFF){
        return readInterruptRegister();
    }
    return readFromHram(address);
}

void writeBus(u16 address, u8 value){
    if(address < 0x8000){
        writeToCartridge(address, value);
        return;
    }
    else if(address < 0xA000){
        writeVram(address, value);
        return;
    }
    else if(address < 0xC000){
        writeToCartridge(address, value);
        return;
    }
    else if(address < 0xE000){
        writeToWram(address, value);
        return;
    }
    else if(address < 0xFE00){
        return;
    }
    else if(address < 0xFEA0){
        if(getDmaContext()->active){
            return;
        }
        writeOam(address, value);
        return;
    }
    else if(address < 0xFF00){
        return;
    }
    else if(address < 0xFF80){
        writeIo(address, value);
        return;
    }
    else if(address == 0xFFFF){
        setInterruptRegister(value);
        return;
    }
    writeToHram(address, value);
}

void writeBus16(u16 address, u16 value){
    writeBus(address + 1, (value >> 8) & 0xFF);
    writeBus(address, value & 0xFF);
}

u16 readBus16(u16 address) {
    u16 low = readBus(address);
    u16 high = readBus(address + 1);

    return low | (high << 8);
}