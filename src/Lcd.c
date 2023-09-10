#include <Lcd.h>
#include <Dma.h>
#include <Ppu.h>
#include <GameBoyEmulator.h>

static lcdContext LCD = {0};

static unsigned long defaultColors[4] = {0xFFFFFFFF, 0xFF999999, 0xFF555555, 0xFF111111}; //{0xFFFBBC0F, 0xFF9BAC0F, 0xFF306230, 0xFF09380F}; 

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
    if(address == 0xFF4B){
        return LCD.winX;
    }
    else if(address == 0xFF4A){
        return LCD.winY;
    }
    else if(address == 0xFF42){
        return LCD.scrollY;
    }
    else if(address == 0xFF43){
        return LCD.scrollX;
    }
    else if(address == 0xFF40){
        return LCD.lcdC; /*% 144*/;
    }
    else if(address == 0xFF41){
        return LCD.lcdS;
    }

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
    if(address == 0xFF4B){
        LCD.winX = value /*% 167*/;
        
        return;
    }
    else if(address == 0xFF4A){
        LCD.winY = value /*% 144*/;
        return;
    }
    else if(address == 0xFF42){
        LCD.scrollY = value /*% 144*/;
        //printf("wrote to scrolly %d\n", value);
        return;
    }
    else if(address == 0xFF43){
        LCD.scrollX = value /*% 144*/;
        return;
    }
    else if(address == 0xFF40){
        LCD.lcdC = value /*% 144*/;
        if(BIT(getLcdContext()->lcdC, 7)){
            //printf("hmm\n");
        }
        else{
            printf("lcd is disabled\n");
        }
        return;
    }
    else if(address == 0xFF41){
        LCD.lcdS = value;
        return;
    }

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
    

    /*if(LCDC_WIN_ENABLE){
        printf("winx: %d  winy: %d  at line %d with lyc = %d\n", LCD.winX, LCD.winY, LCD.lY, LCD.lYCompare);
    }*/
}