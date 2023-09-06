#include <Cpu.h>
#include <Bus.h>
#include <GameBoyEmulator.h>
#include <Stack.h>


registerType registerTypeLookup[] = {
    RT_B,
    RT_C,
    RT_D,
    RT_E,
    RT_H,
    RT_L,
    RT_HL,
    RT_A
};

registerType decodeRegister(u8 reg){
    if(reg > 0b111){
        return RT_NONE;
    }
    return registerTypeLookup[reg];
}

static bool is16Bit(registerType rt){
    return rt >= RT_AF;
}


static void noneProcess(cpuContext *CPU){
    printf("\tERR: INV INST\n");
    char flags[16];
    sprintf(flags, "%c%c%c%c", 
        CPU->registers.f & (1 << 7) ? 'Z' : '-',
        CPU->registers.f & (1 << 6) ? 'N' : '-',
        CPU->registers.f & (1 << 5) ? 'H' : '-',
        CPU->registers.f & (1 << 4) ? 'C' : '-'
    );
    char inst[16];
    instructionToString(CPU, inst);
    printf("INFO: %08lX - 0x%04X: %-12s (0x%02X 0x%02X 0x%02X) A: 0x%02X F: %s BC: 0x%02X%02X DE: 0x%02X%02X HL: 0x%02X%02X DEST: %d FETCHED: 0x%02X\n", 
        GetEmulatorContext()->ticks,
        CPU->registers.pc, inst, CPU->currentOpcode,
        readBus(CPU->registers.pc + 1), readBus(CPU->registers.pc + 2), CPU->registers.a, flags, CPU->registers.b, CPU->registers.c,
        CPU->registers.d, CPU->registers.e, CPU->registers.h, CPU->registers.l, CPU->destinationIsMemory, CPU->fetchedData);

    exit(-31);
}

static void nopProcess(cpuContext *CPU){
    
}

static void ldProcess(cpuContext *CPU){

    if(CPU->destinationIsMemory){
        if(is16Bit(CPU->currentInstruction->reg2)){
            waitForCPUCycle(1);
            writeBus16(CPU->memoryDestination, CPU->fetchedData);
        }
        else{
            writeBus(CPU->memoryDestination, CPU->fetchedData);
            //printf("writing to memory address 0x%04X the value %d\n", CPU->memoryDestination, CPU->fetchedData);
        }
        waitForCPUCycle(1);
        return;
    }

    if(CPU->currentInstruction->mode == AM_HL_SPR){
        u8 hFlag = (readRegister(CPU->currentInstruction->reg2) & 0xF) + (CPU->fetchedData & 0xF) >= 0x10;
        u8 cFlag = (readRegister(CPU->currentInstruction->reg2) & 0xFF) + (CPU->fetchedData & 0xFF) >= 0x100;
        cpuSetFlags(CPU, 0, 0, hFlag, cFlag);
        setRegister(CPU->currentInstruction->reg1, readRegister(CPU->currentInstruction->reg2) + (int8_t)CPU->fetchedData);
        return;
    }

    setRegister(CPU->currentInstruction->reg1, CPU->fetchedData);
}

static void ldhProcess(cpuContext *CPU){
    if(CPU->currentInstruction->reg1 == RT_A){
        setRegister(CPU->currentInstruction->reg1, readBus(0xFF00 | CPU->fetchedData));
    }
    else{
        writeBus(CPU->memoryDestination, CPU->registers.a);
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
    int8_t relative = (int8_t)(CPU->fetchedData & 0xFF);
    //printf("%d\n", relative);
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

static void eiProcess(cpuContext *CPU){
    CPU->enablingInterrupts = true;
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
    u16 low = readRegister(CPU->currentInstruction->reg1) & 0xFF;
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
    cpuSetFlags(CPU, value == 0, 0, (value & 0x0F) == 0, -1);
}

static void decProcess(cpuContext *CPU){
    u16 value = readRegister(CPU->currentInstruction->reg1) - 1;
    if(is16Bit(CPU->currentInstruction->reg1)){
        waitForCPUCycle(1);
    }
    if(CPU->currentInstruction->reg1 == RT_HL && CPU->currentInstruction->mode == AM_MR){
        value = readBus(readRegister(RT_HL)) - 1;
        writeBus(readRegister(RT_HL), value);
    }
    else{
        setRegister(CPU->currentInstruction->reg1, value);
        value = readRegister(CPU->currentInstruction->reg1);
    }

    if((CPU->currentOpcode & 0x0B) == 0x0B){
        return;
    }
    cpuSetFlags(CPU, value == 0, 1, (value & 0x0F) == 0x0F, -1);
}

static void addProcess(cpuContext *CPU){
    u32 value = readRegister(CPU->currentInstruction->reg1) + CPU->fetchedData;

    bool is16 = is16Bit(CPU->currentInstruction->reg1);
    if(is16){
        waitForCPUCycle(1);
    }

    if(CPU->currentInstruction->reg1 == RT_SP){
        value = readRegister(CPU->currentInstruction->reg1) + (int8_t)CPU->fetchedData; 
    }

    int z = (value & 0xFF) == 0;
    int h = (readRegister(CPU->currentInstruction->reg1) & 0xF) + (CPU->fetchedData & 0xF) >= 0x10;
    int c = (int)(readRegister(CPU->currentInstruction->reg1) & 0xFF) + (int)(CPU->fetchedData & 0xFF) >= 0x100;

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
    CPU->registers.a |= (CPU->fetchedData & 0xFF);
    cpuSetFlags(CPU, CPU->registers.a == 0, 0, 0, 0);
}

static void xorProcess(cpuContext *CPU){
    CPU->registers.a ^= (CPU->fetchedData & 0xFF);
    cpuSetFlags(CPU, CPU->registers.a == 0, 0, 0, 0);
}

static void cpProcess(cpuContext *CPU){
    int value = (int)CPU->registers.a - (int)CPU->fetchedData;
    cpuSetFlags(CPU, value == 0, 1, ((int)CPU->registers.a & 0x0F) - ((int)CPU->fetchedData & 0x0F) < 0, value < 0);
}

static void cbProcess(cpuContext *CPU){
    u8 operation = CPU->fetchedData;
    registerType reg = decodeRegister(operation & 0b111);
    u8 bit = (operation >> 3) & 0b111;
    u8 bitOperation = (operation >> 6) & 0b11;
    u8 registerValue = readRegister8(reg);

    waitForCPUCycle(1);
    if(reg == RT_HL){
        waitForCPUCycle(2);
    }

    switch(bitOperation){
        case 1:
            cpuSetFlags(CPU, !(registerValue & (1 << bit)), 0, 1, -1);
            return;
        case 2:
            registerValue &= ~(1 << bit);
            setRegister8(reg, registerValue);
            return;    
        case 3:
            registerValue |= (1 << bit);
            setRegister8(reg, registerValue);
            return;
    }

    bool cFlag = CPU_FLAG_C;

    switch(bit){
        case 0:{
            bool cSet = false;
            u8 result = (registerValue << 1) & 0xFF;
            if((registerValue & (1 << 7)) != 0){
                result |= 1;
                cSet = true;
            }
            setRegister8(reg, result);
            cpuSetFlags(CPU, result == 0, false, false, cSet);
            return;
        }
        case 1:{
            u8 oldValue = registerValue;
            registerValue >>= 1;
            registerValue |= (oldValue << 7);
            setRegister8(reg, registerValue);
            cpuSetFlags(CPU, !registerValue, false, false, oldValue & 1);
            return;
        }
        case 2:{
            u8 oldValue = registerValue;
            registerValue <<= 1;
            registerValue |= cFlag;
            setRegister8(reg, registerValue);
            cpuSetFlags(CPU, !registerValue, false, false, !!(oldValue & 0x80));
            return;
        }
        case 3:{
            u8 oldValue = registerValue;
            registerValue >>= 1;
            registerValue |= (cFlag << 7);
            setRegister8(reg, registerValue);
            cpuSetFlags(CPU, !registerValue, false, false, oldValue & 1);
            return;
        }
        case 4:{
            u8 oldValue = registerValue;
            registerValue <<= 1;
            setRegister8(reg, registerValue);
            cpuSetFlags(CPU, !registerValue, false, false, !!(oldValue & 0x80));
            return;
        }
        case 5:{
            u8 oldValue = (int8_t)registerValue >> 1;
            setRegister8(reg, oldValue);
            cpuSetFlags(CPU, !oldValue, 0, 0, registerValue & 1);
            return;
        }
        case 6:{
            registerValue = ((registerValue & 0xF0) >> 4) | ((registerValue & 0xF) << 4);
            setRegister8(reg, registerValue);
            cpuSetFlags(CPU, registerValue == 0, false, false, false);
            return;
        }
        case 7:{
            u8 oldValue = registerValue >> 1;
            setRegister8(reg, oldValue);
            cpuSetFlags(CPU, !oldValue, 0, 0, registerValue & 1);
            return;
        }
    }
    printf("\tERR: INVALID CB: 0x%02X\n", operation);
    NO_IMPLEMENTATION
}

static void rlcaProcess(cpuContext *CPU){
    u8 value = CPU->registers.a;
    bool c = (value >> 7) & 1;
    value = (value << 1) | c;
    CPU->registers.a = value;
    cpuSetFlags(CPU, 0, 0, 0, c);
}

static void rrcaProcess(cpuContext *CPU){
    u8 value = CPU->registers.a & 1;
    CPU->registers.a >>= 1;
    CPU->registers.a |= (value << 7);
    cpuSetFlags(CPU, 0, 0, 0, value);
}

static void rlaProcess(cpuContext *CPU){
    u8 value = CPU->registers.a;
    u8 cFlag = CPU_FLAG_C;
    u8 c = (value >> 7) & 1;
    CPU->registers.a = (value << 1) | cFlag;
    cpuSetFlags(CPU, 0, 0, 0, c);
}

static void rraProcess(cpuContext *CPU){
    u8 carryFlag = CPU_FLAG_C;
    u8 newCarryFlag = CPU->registers.a & 1;
    CPU->registers.a >>= 1;
    CPU->registers.a |= (carryFlag << 7);
    cpuSetFlags(CPU, 0, 0, 0, newCarryFlag);
}

static void stopProcess(cpuContext *CPU){
    printf("\tERR: CPU STOP\n");
    
    //NO_IMPLEMENTATION
}

static void daaProcess(cpuContext *CPU){
    u8 value = 0;
    int cFlag = 0;

    if (CPU_FLAG_H || (!CPU_FLAG_N && (CPU->registers.a & 0xF) > 9)) {
        value = 6;
    }

    if (CPU_FLAG_C || (!CPU_FLAG_N && CPU->registers.a > 0x99)) {
        value |= 0x60;
        cFlag = 1;
    }

    CPU->registers.a += CPU_FLAG_N ? -value : value;

    cpuSetFlags(CPU, CPU->registers.a == 0, -1, 0, cFlag);
}

static void cplProcess(cpuContext *CPU){
    CPU->registers.a = ~CPU->registers.a;
    cpuSetFlags(CPU, -1, 1, 1, -1);
}

static void scfProcess(cpuContext *CPU){
    cpuSetFlags(CPU, -1, 0, 0, 1);
}

static void ccfProcess(cpuContext *CPU){
    cpuSetFlags(CPU, -1, 0, 0, CPU_FLAG_C ^ 1);
}

static void haltProcess(cpuContext *CPU){
    CPU->halted = true;
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
    [IN_RST] = rstProcess,
    [IN_DEC] = decProcess,
    [IN_INC] = incProcess,
    [IN_ADD] = addProcess,
    [IN_ADC] = adcProcess,
    [IN_SUB] = subProcess,
    [IN_SBC] = sbcProcess,
    [IN_AND] = andProcess,
    [IN_XOR] = xorProcess,
    [IN_OR] = orProcess,
    [IN_CP] = cpProcess,
    [IN_CB] = cbProcess,
    [IN_RRCA] = rrcaProcess,
    [IN_RLCA] = rlcaProcess,
    [IN_RRA] = rraProcess,
    [IN_RLA] = rlaProcess,
    [IN_STOP] = stopProcess,
    [IN_HALT] = haltProcess,
    [IN_DAA] = daaProcess,
    [IN_CPL] = cplProcess,
    [IN_SCF] = scfProcess,
    [IN_CCF] = ccfProcess,
    [IN_EI] = eiProcess,
    [IN_RETI] = retiProcess
};


InstructionProcess instructionGetProcessor(instructionType type){
    return processors[type];
}