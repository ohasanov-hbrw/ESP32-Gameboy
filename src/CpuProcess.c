#include <Cpu.h>
#include <Bus.h>
#include <GameBoyEmulator.h>
#include <Stack.h>


static bool is16Bit(registerType rt){
    return rt >= RT_AF;
}


static void noneProcess(cpuContext *CPU){
    printf("\tERR: INV INST\n");
    exit(-31);
}

static void nopProcess(cpuContext *CPU){
    
}

static void ldProcess(cpuContext *CPU){
    if(CPU->destinationIsMemory){
        if(is16Bit(CPU->currentInstruction->reg2)){
            writeBus16(CPU->memoryDestination, CPU->fetchedData);
            waitForCPUCycle(1);
        }
        else{
            writeBus(CPU->memoryDestination, CPU->fetchedData);
            waitForCPUCycle(1);
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
    bool z = CPU_FLAG_Z;
    bool c = CPU_FLAG_C;

    switch(CPU->currentInstruction->condition){
        case CT_NONE:
            return true;
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

static void goToAddress(cpuContext *CPU, u16 address, bool pushPc){
    if(checkCondition(CPU)){
        if(pushPc){
            waitForCPUCycle(2);
            push16ToStack(CPU->registers.pc);
        }
        CPU->registers.pc = address;
        waitForCPUCycle(1);
    }
}

static void jpProcess(cpuContext *CPU){
    goToAddress(CPU, CPU->fetchedData, false);
}

static void jrProcess(cpuContext *CPU){
    char relative = (char)(CPU->fetchedData & 0xFF);
    u16 address = CPU->registers.pc + relative;
    goToAddress(CPU, address, false);
}

static void callProcess(cpuContext *CPU){
    goToAddress(CPU, CPU->fetchedData, true);
}

static void rstProcess(cpuContext *CPU){
    goToAddress(CPU, CPU->currentInstruction->parameter, true);
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

static void popProcess(cpuContext *CPU){
    u16 low = popFromStack();
    waitForCPUCycle(1);
    u16 high = popFromStack();
    waitForCPUCycle(1);
    u16 value = (high << 8) | low;

    setRegister(CPU->currentInstruction->reg1, value);

    if(CPU->currentInstruction->reg1 == RT_AF){
        setRegister(CPU->currentInstruction->reg1, value & 0xFFF0);
    }
}

static void pushProcess(cpuContext *CPU){
    u16 high = (readRegister(CPU->currentInstruction->reg1) >> 8) & 0xFF;
    waitForCPUCycle(1);
    pushToStack(high);
    u16 low = (readRegister(CPU->currentInstruction->reg1)) & 0xFF;
    waitForCPUCycle(1);
    pushToStack(low);
    waitForCPUCycle(1);
}

static void retProcess(cpuContext *CPU){
    if(CPU->currentInstruction->condition != CT_NONE){
        waitForCPUCycle(1);
    }
    if(checkCondition(CPU)){
        u16 low = popFromStack();
        waitForCPUCycle(1);
        u16 high = popFromStack();
        waitForCPUCycle(1);
        u16 value = (high << 8) | low;
        CPU->registers.pc = value;
        waitForCPUCycle(1);
    }
}

static void retiProcess(cpuContext *CPU){
    CPU->interruptsEnabled = true;
    retProcess(CPU);
}



static void incProcess(cpuContext *CPU){
    u16 value = readRegister(CPU->currentInstruction->reg1) + 1;
    if(is16Bit(CPU->currentInstruction->reg1)){
        waitForCPUCycle(1);
    }
    if(CPU->currentInstruction->reg1 == RT_HL && CPU->currentInstruction->mode == AM_MR){
        value = readBus(readRegister(RT_HL)) + 1;
        value &= 0xFF;
        writeBus(readRegister(RT_HL), value);
    }
    else{
        setRegister(CPU->currentInstruction->reg1, value);
        value = readRegister(CPU->currentInstruction->reg1);
    }

    if((CPU->currentOpcode & 0x03) == 0x03){
        return;
    }
    else{
        cpuSetFlags(CPU, value == 0, 0, (value & 0x0F) == 0, -1);
    }
}

static void decProcess(cpuContext *CPU){
    u16 value = readRegister(CPU->currentInstruction->reg1) - 1;
    if(is16Bit(CPU->currentInstruction->reg1)){
        waitForCPUCycle(1);
    }
    if(CPU->currentInstruction->reg1 == RT_HL && CPU->currentInstruction->mode == AM_MR){
        value = readBus(readRegister(RT_HL)) - 1;
        value &= 0xFF;
        writeBus(readRegister(RT_HL), value);
    }
    else{
        setRegister(CPU->currentInstruction->reg1, value);
        value = readRegister(CPU->currentInstruction->reg1);
    }

    if((CPU->currentOpcode & 0x03) == 0x03){
        return;
    }
    else{
        cpuSetFlags(CPU, value == 0, 1, (value & 0x0F) == 0x0F, -1);
    }
}

static void addProcess(cpuContext *CPU){
    u32 value = readRegister(CPU->currentInstruction->reg1) + CPU->fetchedData;

    bool is16 = is16Bit(CPU->currentInstruction->reg1);
    if(is16){
        waitForCPUCycle(1);
    }
    if(CPU->currentInstruction->reg1 == RT_SP){
        value = readRegister(CPU->currentInstruction->reg1) + (char)CPU->fetchedData; 
    }

    int z = (value & 0xFF) == 0;
    int h = ((readRegister(CPU->currentInstruction->reg1) & 0xF) + (CPU->fetchedData & 0xF)) >= 0x10;
    int c = ((int)(readRegister(CPU->currentInstruction->reg1) & 0xFF) + (int)(CPU->fetchedData & 0xFF)) >= 0x100;

    if(is16){
        z = -1;
        h = ((readRegister(CPU->currentInstruction->reg1) & 0xFFF) + (CPU->fetchedData & 0xFFF)) >= 0x1000;
        u32 i = ((u32)readRegister(CPU->currentInstruction->reg1)) + ((u32)CPU->fetchedData);
        c = i >= 0x10000;
    }

    if(CPU->currentInstruction->reg1 == RT_SP){
        z = 0;
        h = (readRegister(CPU->currentInstruction->reg1) & 0xF) + (CPU->fetchedData & 0xF) >= 0x10;
        c = (int)(readRegister(CPU->currentInstruction->reg1) & 0xFF) + (int)(CPU->fetchedData & 0xFF) >= 0x100;
    }
    setRegister(CPU->currentInstruction->reg1, value & 0xFFFF);
    cpuSetFlags(CPU, z, 0, h, c);
}

static void adcProcess(cpuContext *CPU){
    u16 u = CPU->fetchedData;
    u16 a = CPU->registers.a;
    u16 c = CPU_FLAG_C;

    CPU->registers.a = (a + u + c) & 0xFF;

    cpuSetFlags(CPU, CPU->registers.a == 0, 0, (a & 0xF) + (u & 0xF) + c > 0xF, a + u + c > 0xFF);
}

static void sbcProcess(cpuContext *CPU){
    u8 value = CPU->fetchedData + CPU_FLAG_C;
    int z = readRegister(CPU->currentInstruction->reg1) - value == 0;
    int h = ((int)readRegister(CPU->currentInstruction->reg1) & 0xF) - ((int)CPU->fetchedData & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)readRegister(CPU->currentInstruction->reg1)) - ((int)CPU->fetchedData) - ((int)CPU_FLAG_C) < 0;
    setRegister(CPU->currentInstruction->reg1, readRegister(CPU->currentInstruction->reg1) - value);
    cpuSetFlags(CPU, z, 1, h, c);
}

static void subProcess(cpuContext *CPU){
    u16 value = readRegister(CPU->currentInstruction->reg1) - CPU->fetchedData;

    int z = value == 0;
    int h = ((int)readRegister(CPU->currentInstruction->reg1) & 0xF) - ((int)CPU->fetchedData & 0xF) < 0;
    int c = ((int)readRegister(CPU->currentInstruction->reg1)) - ((int)CPU->fetchedData) < 0;

    setRegister(CPU->currentInstruction->reg1, value);
    cpuSetFlags(CPU, z, 1, h, c);
}

static void andProcess(cpuContext *CPU){
    CPU->registers.a &= CPU->fetchedData;
    cpuSetFlags(CPU, CPU->registers.a == 0, 0, 1, 0);
}

static void orProcess(cpuContext *CPU){
    CPU->registers.a |= CPU->fetchedData && 0xFF;
    cpuSetFlags(CPU, CPU->registers.a == 0, 0, 0, 0);
}

static void xorProcess(cpuContext *CPU){
    CPU->registers.a ^= CPU->fetchedData && 0xFF;
    cpuSetFlags(CPU, CPU->registers.a == 0, 0, 0, 0);
}

static void cpProcess(cpuContext *CPU){
    int value = (int)CPU->registers.a - (int)CPU->fetchedData;
    cpuSetFlags(CPU, value == 0, 1, ((int)CPU->registers.a & 0x0F) - ((int)CPU->fetchedData & 0x0F) < 0, value < 0);
}

static InstructionProcess processors[] = {
    [IN_NONE] = noneProcess,
    [IN_NOP] = nopProcess,
    [IN_LD] = ldProcess,
    [IN_LDH] = ldhProcess,
    [IN_JP] = jpProcess,
    [IN_DI] = diProcess,
    [IN_POP] = popProcess,
    [IN_PUSH] = pushProcess,
    [IN_JR] = jrProcess,
    [IN_CALL] = callProcess,
    [IN_RET] = retProcess,
    [IN_RETI] = retiProcess,
    [IN_RST] = rstProcess,
    [IN_INC] = incProcess,
    [IN_DEC] = decProcess,
    [IN_ADD] = addProcess,
    [IN_ADC] = adcProcess,
    [IN_SBC] = sbcProcess,
    [IN_SUB] = subProcess,
    [IN_OR] = orProcess,
    [IN_XOR] = xorProcess,
    [IN_AND] = andProcess,
    [IN_CP] = cpProcess,
};


InstructionProcess instructionGetProcessor(instructionType type){
    return processors[type];
}