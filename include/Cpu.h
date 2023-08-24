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
    bool enablingInterrupts;

    u8 currentOpcode;

    instruction *currentInstruction;
    
    bool halted;
    bool steppingMode;

    u8 interruptRegister;
    u8 interruptFlags;
}cpuContext;

void initCpu();
bool stepCpu();

typedef void (*InstructionProcess) (cpuContext *);

InstructionProcess instructionGetProcessor(instructionType type);

#define CPU_FLAG_Z BIT(CPU->registers.f, 7)
#define CPU_FLAG_N BIT(CPU->registers.f, 6)
#define CPU_FLAG_H BIT(CPU->registers.f, 5)
#define CPU_FLAG_C BIT(CPU->registers.f, 4)

void cpuSetFlags(cpuContext *, char , char , char , char );

u16 readRegister(registerType);
void setRegister(registerType, u16);

u8 readRegister8(registerType);
void setRegister8(registerType, u8);

cpuRegisters* getRegisters();
 
u8 readInterruptRegister();
void setInterruptRegister(u8);


u8 readInterruptFlags();
void setInterruptFlags(u8);