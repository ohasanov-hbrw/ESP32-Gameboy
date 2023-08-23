#include <Cpu.h>
#include <Bus.h>
#include <GameBoyEmulator.h>

cpuContext CPU = {0};

void initCpu(){
    CPU.registers.pc = 0x100;
    CPU.registers.a = 0x01;
}

static void fetchInstruction(){
    CPU.currentOpcode = readBus(CPU.registers.pc);
    u16 pc = CPU.registers.pc;
    CPU.registers.pc++;
    CPU.currentInstruction = getInstructionFromOpcode(CPU.currentOpcode);

    if(CPU.currentInstruction == NULL){
        printf("\tERR: UNAV OPCD: 0x%02X PC: 0x%04X\n", CPU.currentOpcode, pc);
        exit(-31);
    }
}



static void execute(){
    //printf("\tTODO: EXEC NOT IMPL\n");
    InstructionProcess process = instructionGetProcessor(CPU.currentInstruction->type);
    if(!process){
        printf("\tERR: UNAV PROC TYPE: 0x%02X\n", CPU.currentOpcode);
        NO_IMPLEMENTATION
    }
    process(&CPU);
}


bool stepCpu(){
    if(!CPU.halted){
        u16 pc = CPU.registers.pc;
        fetchInstruction();
        fetchData();
        printf("INFO: EXEC INST PC: 0x%04X NAME:%8s (0x%02X 0x%02X 0x%02X) A: 0x%02X BC: 0x%02X 0x%02X DE: 0x%02X 0x%02X HL: 0x%02X 0x%02X\n", pc, instructionName(CPU.currentInstruction->type), CPU.currentOpcode, readBus(pc + 1), readBus(pc + 2), CPU.registers.a, CPU.registers.b, CPU.registers.c, CPU.registers.d, CPU.registers.e, CPU.registers.h, CPU.registers.l);
        execute();
        if(pc > 0x4000)
            exit(0);
    }
    
    return true;
}

u8 readInterruptRegister(){
    return CPU.interruptRegister;
}
void setInterruptRegister(u8 value){
    CPU.interruptRegister = value;
}