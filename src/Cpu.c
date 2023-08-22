#include <Cpu.h>
#include <Bus.h>

cpuContext CPU = {0};

void initCpu(){

}

static void fetchInstruction(){
    CPU.currentOpcode = readBus(CPU.registers.pc);
    CPU.registers.pc++;
    CPU.currentInstruction = getInstructionFromOpcode(CPU.currentOpcode);

    if(CPU.currentInstruction == NULL){
        printf("unavaliable instruction on opcode: %02X \n", CPU.currentOpcode);
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
            printf("unknown addressing mode: %d \n", CPU.currentInstruction->mode);
            exit(-31);
            return;
    }
}

static void execute(){
    printf("executing instructions is not implemented yet!\n");
}

bool stepCpu(){
    if(!CPU.halted){
        fetchInstruction();
        fetchData();
        execute();
    }
    
    return false;
}