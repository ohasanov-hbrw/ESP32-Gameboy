#include <Cpu.h>

extern cpuContext CPU;

u16 reverseByte(u16 n) {
    return ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
}


u16 readRegister(registerType rt){
    switch(rt){
        case RT_A: return CPU.registers.a;
        case RT_F: return CPU.registers.f;
        case RT_B: return CPU.registers.b;
        case RT_C: return CPU.registers.c;
        case RT_D: return CPU.registers.d;
        case RT_E: return CPU.registers.e;
        case RT_H: return CPU.registers.h;
        case RT_L: return CPU.registers.l;

        case RT_AF: return reverseByte(*((u16 *)&CPU.registers.a));
        case RT_BC: return reverseByte(*((u16 *)&CPU.registers.b));
        case RT_DE: return reverseByte(*((u16 *)&CPU.registers.d));
        case RT_HL: return reverseByte(*((u16 *)&CPU.registers.h));

        case RT_PC: return CPU.registers.pc;
        case RT_SP: return CPU.registers.sp;
        default: return 0;
    }
}

void setRegister(registerType rt, u16 val){
    switch(rt) {
        case RT_A: CPU.registers.a = val & 0xFF; break;
        case RT_F: CPU.registers.f = val & 0xFF; break;
        case RT_B: CPU.registers.b = val & 0xFF; break;
        case RT_C:{
             CPU.registers.c = val & 0xFF;
             break;
        }
        case RT_D: CPU.registers.d = val & 0xFF; break;
        case RT_E: CPU.registers.e = val & 0xFF; break;
        case RT_H: CPU.registers.h = val & 0xFF; break;
        case RT_L: CPU.registers.l = val & 0xFF; break;

        case RT_AF: *((u16 *)&CPU.registers.a) = reverseByte(val); break;
        case RT_BC: *((u16 *)&CPU.registers.b) = reverseByte(val); break;
        case RT_DE: *((u16 *)&CPU.registers.d) = reverseByte(val); break;
        case RT_HL:{
            *((u16 *)&CPU.registers.h) = reverseByte(val); 
            break;
        }

        case RT_PC: CPU.registers.pc = val; break;
        case RT_SP: CPU.registers.sp = val; break;
        case RT_NONE: break;
    }
}

cpuRegisters* getRegisters(){
    return &CPU.registers;
}