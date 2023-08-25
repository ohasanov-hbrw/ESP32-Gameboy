#pragma once

#include <Utils.h>

typedef struct{
    u16 div;
    u8 tima;
    u8 tma;
    u8 tac;
}timerContext;


void initTimer();
void stepTimer();

void writeTimer(u16, u8);
u8 readTimer(u16);

timerContext* getTimerContext();