#include <Interrupts.h>
#include <Stack.h>
#include <Cpu.h>

void handle(cpuContext * CPU, u16 address){
    push16ToStack(CPU->registers.pc);
    CPU->registers.pc = address;
}

bool interruptCheck(cpuContext * CPU, u16 address, interruptType t){
    if(CPU->interruptFlags & t && CPU->interruptRegister & t){
        /*if(t == IT_VBLANK){
            printf("VBLANK interrupt\n");
        }
        else if(t == IT_LCD_STAT){
            printf("LCD STAT interrupt\n");
        }
        else if(t == IT_TIMER){
            printf("TIMER interrupt\n");
        }
        else if(t == IT_SERIAL){
            printf("SERIAL interrupt\n");
        }
        else if(t == IT_JOYPAD){
            printf("JOYPAD interrupt\n");
        }*/
        handle(CPU, address);
        CPU->interruptFlags &= ~t;
        CPU->halted = false;
        CPU->interruptsEnabled = false;
        return true;
    }
    return false;
}


void handleInterrupts(cpuContext * CPU){
    if(interruptCheck(CPU, 0x40, IT_VBLANK));
    else if(interruptCheck(CPU, 0x48, IT_LCD_STAT));
    else if(interruptCheck(CPU, 0x50, IT_TIMER));
    else if(interruptCheck(CPU, 0x58, IT_SERIAL));
    else if(interruptCheck(CPU, 0x60, IT_JOYPAD));
}