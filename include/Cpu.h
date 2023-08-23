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

    bool interruptsEnabled;

    u8 currentOpcode;

    instruction *currentInstruction;
    
    bool halted;
    bool steppingMode;

    u8 interruptRegister;

}cpuContext;

void initCpu();
bool stepCpu();

typedef void (*InstructionProcess) (cpuContext *);

InstructionProcess instructionGetProcessor(instructionType type);

#define CPUFLAGZ BIT(CPU->registers.f, 7)
#define CPUFLAGC BIT(CPU->registers.f, 4)

void cpuSetFlags(cpuContext *, char , char , char , char );

u16 readRegister(registerType);
void setRegister(registerType, u16);

cpuRegisters* getRegisters();

u8 readInterruptRegister();
void setInterruptRegister(u8);