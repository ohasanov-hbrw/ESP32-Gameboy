#pragma once
#include <Utils.h>

u8 readFromWram(u16 );
void writeToWram(u16, u8);
u8 readFromHram(u16);
void writeToHram(u16, u8);