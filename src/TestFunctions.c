#include <Cpu.h>
#include <Ui.h>
#include <Bus.h>
#include <GameBoyEmulator.h>
#include <Interrupts.h>
#include <DebugFuncs.h>
#include <GBRam.h>
#include <Timer.h>


registerType registerTypeLookupTable[] = {
    RT_B,
    RT_C,
    RT_D,
    RT_E,
    RT_H,
    RT_L,
    RT_HL,
    RT_A
};

registerType decodeRegisterFromData(u8 reg){
    if(reg > 0b111){
        return RT_NONE;
    }
    return registerTypeLookupTable[reg];
}

//0x00
static void nop(cpuContext *CPU){
    waitForCPUCycle(1);
}

//0x01
static void ld_bc_u16(cpuContext *CPU){
    u16 low = readBus(CPU->registers.pc);
    u16 high = readBus(CPU->registers.pc + 1);
    CPU->registers.pc += 2;
    waitForCPUCycle(1);
    setRegister(RT_C, low);
    waitForCPUCycle(1);
    setRegister(RT_B, high);
    waitForCPUCycle(1);
}

//0x02
static void ld_bc_a(cpuContext *CPU){
    u16 data = readRegister(RT_A);
    waitForCPUCycle(1);
    setRegister(RT_B, data);
    waitForCPUCycle(1);
}

//0x03
static void inc_bc(cpuContext *CPU){
    u16 data = readRegister(RT_BC) + 1;
    waitForCPUCycle(1);
    setRegister(RT_BC, data);
    waitForCPUCycle(1);
}

//0x04
static void inc_b(cpuContext *CPU){
    u8 data = readRegister8(RT_B) + 1;
    waitForCPUCycle(1);
    setRegister8(RT_B, data);
    cpuSetFlags(CPU, data == 0, 0, (data & 0x0F) == 0, -1);
}

//0x05
static void dec_b(cpuContext *CPU){
    u8 data = readRegister8(RT_B) - 1;
    setRegister8(RT_B, data);
    cpuSetFlags(CPU, data == 0, 1, (data & 0x0F) == 0x0F, -1);
    waitForCPUCycle(1);
}

//0x06
static void ld_b_u8(cpuContext *CPU){
    u8 data = readBus(CPU->registers.pc);
    waitForCPUCycle(1);
    setRegister8(RT_B, data);
    waitForCPUCycle(1);
}

//0x07
static void rlca(cpuContext *CPU){
    u8 data = CPU->registers.a;
    bool c = (data >> 7) & 1;
    data = (data << 1) | c;
    CPU->registers.a = data;
    cpuSetFlags(CPU, 0, 0, 0, c);
    waitForCPUCycle(1);
}