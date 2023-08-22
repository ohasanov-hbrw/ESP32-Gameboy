#include <Cpu.h>
#include <Bus.h>
#include <GameBoyEmulator.h>

cpuContext CPU = {0};

void initCpu(){
    CPU.registers.pc = 0x100;
}

static void fetchInstruction(){
    CPU.currentOpcode = readBus(CPU.registers.pc);

    CPU.registers.pc++;
    CPU.currentInstruction = getInstructionFromOpcode(CPU.currentOpcode);

    if(CPU.currentInstruction == NULL){
        printf("\tERR: UNAV OPCD: 0x%02X \n", CPU.currentOpcode);
        exit(-31);
    }
}

static void fetchData(){
    CPU.memoryDestination = 0;
    CPU.destinationIsMemory = false;

    switch(CPU.currentInstruction->mode){
        case AM_IMP:
            return;
        case AM_R:
            CPU.fetchedData = readRegister(CPU.currentInstruction->reg1);
            return;
        case AM_R_D8:
            CPU.fetchedData = readBus(CPU.registers.pc);
            waitForCPUCycle(1);
            CPU.registers.pc++;
            return;
        case AM_D16:{
            u16 value = readBus(CPU.registers.pc) | (readBus(CPU.registers.pc + 1) << 8);
            waitForCPUCycle(2);
            CPU.fetchedData = value;
            CPU.registers.pc += 2;
            return;
        }
        default:
            printf("\tERR: UNKN ADDR MODE: %d \n", CPU.currentInstruction->mode);
            exit(-31);
            return;
    }
}

static void execute(){
    printf("\tTODO: EXEC NOT IMPL\n");
}

bool stepCpu(){
    if(!CPU.halted){
        u16 pc = CPU.registers.pc;
        fetchInstruction();
        fetchData();
        printf("INFO: EXEC INST: 0x%02X  PC: 0x%04X\n", CPU.currentOpcode, pc);
        execute();
    }
    
    return true;
}