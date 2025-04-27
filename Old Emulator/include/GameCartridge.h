#pragma once

#include <Utils.h>

typedef struct{
    u8 entry[4];
    u8 logo[0x30];

    char title[16];
    u16 newLicCode;
    u8 sgbFlag;
    u8 type;
    u8 romSize;
    u8 ramSize;
    u8 destCode;
    u8 licCode;
    u8 version;
    u8 checksum;
    u16 globalChecksum;
}romHeader;

bool loadCartridge(char *cart);

u8 readFromCartridge(u16 address);
void writeToCartridge(u16 address, u8 value);