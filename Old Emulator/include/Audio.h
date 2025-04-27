#pragma once

#include <Utils.h>

typedef struct{
    u8 nr52;
    u8 nr51;
    u8 nr50;
}audioContext;


audioContext *getAudioContext();

u8 readAudio(u16 address);

void writeAudio(u16 address, u8 value);

void initAudio();

void tickAudio();