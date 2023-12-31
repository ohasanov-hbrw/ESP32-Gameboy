#include <Cpu.h>
#include <Ui.h>
#include <Bus.h>
#include <GameBoyEmulator.h>
#include <Interrupts.h>
#include <DebugFuncs.h>
#include <GBRam.h>
#include <Timer.h>

int amogus = 0;
int lastamogus = 0;


#define DEBUG_CPU 1

cpuContext CPU = {0};

void initCpu(){
    cleanRam();
    CPU.registers.pc = 0x100;
    CPU.registers.sp = 0xFFFE;
    *((short *)&CPU.registers.a) = 0xB001;
    *((short *)&CPU.registers.b) = 0x1300;
    *((short *)&CPU.registers.d) = 0xD800;
    *((short *)&CPU.registers.h) = 0x4D01;
    CPU.registers.a = 0x01;
    CPU.interruptRegister = 0;
    CPU.interruptFlags = 0;
    CPU.interruptsEnabled = false;
    CPU.enablingInterrupts = false;
    getTimerContext()->div = 0xABCC;
    CPU.execAgain = false;
}

static void fetchInstruction(){
    CPU.currentOpcode = readBus(CPU.registers.pc);
    incrementPc();
    CPU.currentInstruction = getInstructionFromOpcode(CPU.currentOpcode);
}

void incrementPc(){
    /*if(CPU.execAgain){
        CPU.execAgain = false;
    }
    else{*/
        CPU.registers.pc++;
    //}
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
        CPU.currentOpcode = readBus(CPU.registers.pc);
        incrementPc();
        CPU.currentInstruction = getInstructionFromOpcode(CPU.currentOpcode);
        /*waitOneCycle();
        fetchInstruction();
        waitThreeCycles();
        //waitForCPUCycle(1);

        fetchData();*/

#if DEBUG_CPU == 1
        if(GetEmulatorContext()->debug){
            char flags[16];
            sprintf(flags, "%c%c%c%c", 
                CPU.registers.f & (1 << 7) ? 'Z' : '-',
                CPU.registers.f & (1 << 6) ? 'N' : '-',
                CPU.registers.f & (1 << 5) ? 'H' : '-',
                CPU.registers.f & (1 << 4) ? 'C' : '-'
            );
            char inst[16];
            instructionToString(&CPU, inst);
            printf("INFO: %08lX - %04X: %-12s (%02X %02X %02X) A: %02X F: %s BC: %02X%02X DE: %02X%02X HL: %02X%02X\n", 
                GetEmulatorContext()->ticks,
                pc, inst, CPU.currentOpcode,
                readBus(pc + 1), readBus(pc + 2), CPU.registers.a, flags, CPU.registers.b, CPU.registers.c,
                CPU.registers.d, CPU.registers.e, CPU.registers.h, CPU.registers.l);
            //if(CPU.currentOpcode == 0x76) exit(-8);
            //if(pc == 0x0B7F) exit(-8);
            updateDebug();
            //printDebug();
        }
#endif
        /*execute();*/
        //exit(0);
        /*if(CPU.currentOpcode == 0xFF){
            printf("wtf\n");
            exit(0);

        }*/
        AccInstructionProcess accProcess = accInstructionGetProcessor(CPU.currentOpcode);
        if(!accProcess){
            printf("\tERR: UNAV PROC TYPE: 0x%02X\n", CPU.currentOpcode);
            NO_IMPLEMENTATION
        }
        accProcess(&CPU);
        
    }
    else{
        waitForCPUCycle(1);
        if(CPU.interruptFlags){
            CPU.halted = false;
        }
    }
    
    if(CPU.interruptsEnabled){
        handleInterrupts(&CPU);
        CPU.enablingInterrupts = false;
    }

    if(CPU.enablingInterrupts){
        CPU.interruptsEnabled = true;
    }

    return true;
}

u8 readInterruptRegister(){
    return CPU.interruptRegister;
}
void setInterruptRegister(u8 value){
    CPU.interruptRegister = value;
}

void requestInterrupt(interruptType t){
    CPU.interruptFlags |= t;
}