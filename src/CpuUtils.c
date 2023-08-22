#include <Cpu.h>

extern cpuContext CPU;

u16 reverseByte(u16 n) {
    return ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
}


u16 readRegister(registerType rt) {
    switch(rt) {
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