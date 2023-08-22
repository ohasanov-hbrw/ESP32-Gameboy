#pragma once

#include <Utils.h>
#include <Instructions.h>

typedef struct{
    u8 a;
    u8 f;
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 h;
    u8 l;
    u16 pc;
    u16 sp;
}cpuRegisters;

typedef struct{
    cpuRegisters registers;

    u16 fetchedData;
    u16 memoryDestination;

    bool destinationIsMemory;

    u8 currentOpcode;

    instruction *currentInstruction;
    
    bool halted;
    bool steppingMode;

}cpuContext;

void initCpu();
bool stepCpu();