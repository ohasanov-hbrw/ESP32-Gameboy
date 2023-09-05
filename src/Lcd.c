#include <Lcd.h>
#include <Dma.h>
#include <Ppu.h>

static lcdContext LCD = {0};

static unsigned long defaultColors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000}; 

lcdContext *getLcdContext(){
    return &LCD;
}

void initLcd(){
    LCD.lcdC = 0x91;
    LCD.scrollX = 0;
    LCD.scrollY = 0;
    LCD.lY = 0;
    LCD.lYCompare = 0;
    LCD.bgp = 0xFC;
    LCD.obp[0] = 0xFF;
    LCD.obp[1] = 0xFF;
    LCD.winY = 0;
    LCD.winX = 0;
    for(int i = 0; i < 4; i++){
        LCD.bgc[i] = defaultColors[i];
        LCD.spc1[i] = defaultColors[i];
        LCD.spc2[i] = defaultColors[i];
    }
}


u8 readLcd(u16 address){
    u8 offset = (address - 0xFF40);
    u8 *p = (u8 *)&LCD;
    //printf("reading from lcd\n");
    return p[offset];
}

void updatePalette(u8 pdata, u8 pal) {
    u32 *colors = LCD.bgc;
    switch(pal){
        case 1:
            colors = LCD.spc1;
            break;
        case 2:
            colors = LCD.spc2;
            break;
    }
    colors[0] = defaultColors[pdata & 0b11];
    colors[1] = defaultColors[(pdata >> 2) & 0b11];
    colors[2] = defaultColors[(pdata >> 4) & 0b11];
    colors[3] = defaultColors[(pdata >> 6) & 0b11];
}


void writeLcd(u16 address, u8 value){
    u8 offset = (address - 0xFF40);
    u8 *p = (u8 *)&LCD;
    p[offset] = value;

    //printf("writing to lcd\n");
    if(offset == 6){ 
        startDma(value);
    }
    else if(address == 0xFF47){
        updatePalette(value, 0);
    }
    else if(address == 0xFF48){
        updatePalette(value & 0b11111100, 1);
    }
    else if(address == 0xFF49){
        updatePalette(value & 0b11111100, 2);
    }
    else if(address == 0xFF4B){
        LCD.winX = value /*% 167*/;
        //printf("winx: %d  at line %d with lyc = %d\n", LCD.winX, LCD.lY, LCD.lYCompare);
    }
    else if(address == 0xFF4A){
        LCD.winY = value /*% 144*/;
        //printf("winy: %d  at line %d with lyc = %d\n", LCD.winY, LCD.lY, LCD.lYCompare);
    }

    /*if(LCDC_WIN_ENABLE){
        printf("winx: %d  winy: %d  at line %d with lyc = %d\n", LCD.winX, LCD.winY, LCD.lY, LCD.lYCompare);
    }*/
}