#pragma once //
#include <Utils.h>

typedef struct{
    bool active;
    u8 byte; //
    u8 value;
    u8 delay;
}dmaContext;



void startDma(u8);
void stepDma();
bool isTransferingDma();

dmaContext* getDmaContext();