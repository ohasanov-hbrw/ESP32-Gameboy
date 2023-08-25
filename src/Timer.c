#include <Timer.h>
#include <Interrupts.h>

static timerContext TIMER = {0};

void initTimer(){
    TIMER.div = 0xAC00;
}

void stepTimer(){
    u16 previousDiv = TIMER.div;
    TIMER.div++;
    bool updateTimer = false;
    switch(TIMER.tac & (0b11)){
        case 0b00:
            updateTimer = (previousDiv & (1 << 9)) && (!(TIMER.div & (1 << 9)));
            break;
        case 0b01:
            updateTimer = (previousDiv & (1 << 3)) && (!(TIMER.div & (1 << 3)));
            break;
        case 0b10:
            updateTimer = (previousDiv & (1 << 5)) && (!(TIMER.div & (1 << 5)));
            break;
        case 0b11:
            updateTimer = (previousDiv & (1 << 7)) && (!(TIMER.div & (1 << 7)));
            break;
    }
    if(updateTimer && TIMER.tac & (1 << 2)){
        TIMER.tima++;
        if(TIMER.tima == 0xFF){
            TIMER.tima = TIMER.tma;
            requestInterrupt(IT_TIMER);
        }
    }
}

void writeTimer(u16 address, u8 value){
    switch(address){
        case 0xFF04:
            TIMER.div = 0;
            break;
        case 0xFF05:
            TIMER.tima = value;
            break;
        case 0xFF06:
            TIMER.tma = value;
            break;
        case 0xFF07:
            TIMER.tac = value;
            break;
    }
}

u8 readTimer(u16 address){
    switch(address){
        case 0xFF04:
            return TIMER.div >> 8;
        case 0xFF05:
            return TIMER.tima;
        case 0xFF06:
            return TIMER.tma;
        case 0xFF07:
            return TIMER.tac;
    }
}

timerContext* getTimerContext(){
    return &TIMER;
}