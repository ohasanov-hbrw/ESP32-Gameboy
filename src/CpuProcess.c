#include <Cpu.h>
#include <Bus.h>
#include <GameBoyEmulator.h>

static void noneProcess(cpuContext *CPU){
    printf("\tERR: INV INST\n");
    exit(-31);
}

static void nopProcess(cpuContext *CPU){
    
}

static void ldProcess(cpuContext *CPU){
    if(CPU->destinationIsMemory){
        if(CPU->currentInstruction->reg2 >= RT_AF){
            writeBus16(CPU->memoryDestination, CPU->fetchedData);
            waitForCPUCycle(1);
        }
        else{
            writeBus(CPU->memoryDestination, CPU->fetchedData);
        }
        return;
    }
    if(CPU->currentInstruction->mode == AM_HL_SPR){
        u8 hFlag = (readRegister(CPU->currentInstruction->reg2) & 0xF) + (CPU->fetchedData & 0xF) >= 0x10;
        u8 cFlag = (readRegister(CPU->currentInstruction->reg2) & 0xFF) + (CPU->fetchedData & 0xFF) >= 0x100;
        cpuSetFlags(&CPU, 0, 0, hFlag, cFlag);
        setRegister(CPU->currentInstruction->reg1, readRegister(CPU->currentInstruction->reg2) + (char)CPU->fetchedData);
        return;
    }
    setRegister(CPU->currentInstruction->reg1, CPU->fetchedData);
}

static void ldhProcess(cpuContext *CPU){
    if(CPU->currentInstruction->reg1 == RT_A){
        setRegister(CPU->currentInstruction->reg1, readBus(0xFF00 | CPU->fetchedData));
    }
    else{
        writeBus(0xFF00 | CPU->fetchedData, CPU->registers.a);
    }
    waitForCPUCycle(1);
}

static bool checkCondition(cpuContext *CPU){
    bool z = CPUFLAGZ;
    bool c = CPUFLAGC;

    switch(CPU->currentInstruction->condition){
        case CT_NONE:
            return;
        case CT_C:
            return c;
        case CT_NC:
            return !c;
        case CT_Z:
            return z;
        case CT_NZ:
            return !z;
    }

    return false;
}

static void jpProcess(cpuContext *CPU){
    if(checkCondition(CPU)){
        CPU->registers.pc = CPU->fetchedData;
        waitForCPUCycle(1);
    }
}

static void diProcess(cpuContext *CPU){
    CPU->interruptsEnabled = false;
}

void cpuSetFlags(cpuContext *CPU, char z, char n, char h, char c){
    if(z != -1){
        BIT_SET(CPU->registers.f, 7, z);
    }
    if(n != -1){
        BIT_SET(CPU->registers.f, 6, n);
    }
    if(h != -1){
        BIT_SET(CPU->registers.f, 5, h);
    }
    if(c != -1){
        BIT_SET(CPU->registers.f, 4, c);
    }
}

static void xorProcess(cpuContext *CPU){
    CPU->registers.a ^= CPU->fetchedData & 0xFF;
    cpuSetFlags(CPU, CPU->registers.a == 0, 0, 0, 0);
}

static InstructionProcess processors[] = {
    [IN_NONE] = noneProcess,
    [IN_NOP] = nopProcess,
    [IN_LD] = ldProcess,
    [IN_LDH] = ldhProcess,
    [IN_JP] = jpProcess,
    [IN_DI] = diProcess,
    [IN_XOR] = xorProcess
};


InstructionProcess instructionGetProcessor(instructionType type){
    return processors[type];
}