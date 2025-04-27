#pragma once //
#include <Utils.h>

typedef struct{
    u8 lcdC;
    u8 lcdS;
    u8 scrollY;
    u8 scrollX;
    u8 lY;
    u8 lYCompare;
    u8 dma;
    u8 bgp;
    u8 obp[2];
    u8 winY;
    u8 winX;
    u32 bgc[4];
    u32 spc1[4];
    u32 spc2[4];
}lcdContext;

typedef enum {
    MODE_HBLANK,
    MODE_VBLANK,
    MODE_OAM,
    MODE_XFER
}lcdMode;

lcdContext *getLcdContext();

#define LCDC_BGW_ENABLE (BIT(getLcdContext()->lcdC, 0))
#define LCDC_OBJ_ENABLE (BIT(getLcdContext()->lcdC, 1))
#define LCDC_OBJ_HEIGHT (BIT(getLcdContext()->lcdC, 2) ? 16 : 8)
#define LCDC_BG_MAP_AREA (BIT(getLcdContext()->lcdC, 3) ? 0x9C00 : 0x9800)
#define LCDC_BGW_DATA_AREA (BIT(getLcdContext()->lcdC, 4) ? 0x8000 : 0x8800)
#define LCDC_WIN_ENABLE (BIT(getLcdContext()->lcdC, 5))
#define LCDC_WIN_MAP_AREA (BIT(getLcdContext()->lcdC, 6) ? 0x9C00 : 0x9800)
#define LCDC_LCD_ENABLE (BIT(getLcdContext()->lcdC, 7))

#define LCDS_MODE ((lcdMode)(getLcdContext()->lcdS & 0b11))
#define LCDS_MODE_SET(mode) { getLcdContext()->lcdS &= ~0b11; getLcdContext()->lcdS |= mode; }

#define LCDS_LYC (BIT(getLcdContext()->lcdS, 2))
#define LCDS_LYC_SET(b) (BIT_SET(getLcdContext()->lcdS, 2, b))

#define LCD_CHECK_WINDOW_STATE (LCDC_WIN_ENABLE && getLcdContext()->winX >= 0 && getLcdContext()->winX <= 166 && getLcdContext()->winY >= 0 && getLcdContext()->winY < YRES && getLcdContext()->lY >= getLcdContext()->winY && getLcdContext()->lY < getLcdContext()->winY + YRES)

typedef enum {
    SS_HBLANK = (1 << 3),
    SS_VBLANK = (1 << 4),
    SS_OAM = (1 << 5),
    SS_LYC = (1 << 6),
}statSrc;

#define LCDS_STAT_INT(src) (getLcdContext()->lcdS & src)

void initLcd();

u8 readLcd(u16);
void writeLcd(u16, u8);