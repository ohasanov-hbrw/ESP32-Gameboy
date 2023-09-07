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
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    CPU->registers.pc++;
    setRegister(RT_C, CPU->fetchedData);
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    CPU->registers.pc++;
    setRegister(RT_B, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x02
static void ld_mbc_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    writeBus(readRegister(RT_BC), CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x03
static void inc_bc(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_BC) + 1;
    setRegister(RT_BC, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x04
static void inc_b(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_B) + 1;
    setRegister(RT_B, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 0, (CPU->fetchedData & 0x0F) == 0, -1);
}

//0x05
static void dec_b(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_B) - 1;
    setRegister(RT_B, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 1, (CPU->fetchedData & 0x0F) == 0x0F, -1);
}

//0x06
static void ld_b_u8(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    setRegister(RT_B, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x07
static void rlca(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = CPU->registers.a;
    bool c = (CPU->fetchedData >> 7) & 1;
    CPU->fetchedData = (CPU->fetchedData << 1) | c;
    CPU->registers.a = CPU->fetchedData;
    cpuSetFlags(CPU, 0, 0, 0, c);
}

//0x08
static void ld_mu16_sp(cpuContext *CPU){
    waitForCPUCycle(1);
    u16 low = readBus(CPU->registers.pc);
    CPU->registers.pc++;
    waitForCPUCycle(1);
    u16 high = readBus(CPU->registers.pc);
    CPU->registers.pc++;
    CPU->memoryDestination = low | (high << 8);
    waitForCPUCycle(1);
    writeBus(CPU->memoryDestination, readRegister(RT_SP) & 0xFF);
    waitForCPUCycle(1);
    writeBus(CPU->memoryDestination + 1, (readRegister(RT_SP) >> 8) & 0xFF);
    waitForCPUCycle(1);
}

//0x09
static void add_hl_bc(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_BC);
    u32 value = readRegister(RT_HL) + CPU->fetchedData;
    int h = ((readRegister(RT_HL) & 0xFFF) + (CPU->fetchedData & 0xFFF)) >= 0x1000;
    u32 i = ((u32)readRegister(RT_HL)) + ((u32)CPU->fetchedData);
    int c = i >= 0x10000;
    setRegister(RT_HL, value & 0xFFFF);
    cpuSetFlags(CPU, -1, 0, h, c);
    waitForCPUCycle(1);
}

//0x0A
static void ld_a_mbc(cpuContext *CPU){
    waitForCPUCycle(1);
    setRegister(RT_A, readBus(readRegister(RT_BC)));
    waitForCPUCycle(1);
}

//0x0B
static void dec_bc(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_BC) - 1;
    setRegister(RT_BC, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 1, (CPU->fetchedData & 0x0F) == 0x0F, -1);
    waitForCPUCycle(1);
}

//0x0C
static void inc_c(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_C) + 1;
    setRegister(RT_C, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 0, (CPU->fetchedData & 0x0F) == 0, -1);
}

//0x0D
static void dec_c(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_C) - 1;
    setRegister(RT_C, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 1, (CPU->fetchedData & 0x0F) == 0x0F, -1);
}

//0x0E
static void ld_c_u8(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    setRegister(RT_C, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x0F
static void rrca(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = CPU->registers.a & 1;
    CPU->registers.a >>= 1;
    CPU->registers.a |= (CPU->fetchedData << 7);
    cpuSetFlags(CPU, 0, 0, 0, CPU->fetchedData);
}

//0x10
static void stop(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->stop = true;
}

//0x11
static void ld_de_u16(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    CPU->registers.pc++;
    setRegister(RT_E, CPU->fetchedData);
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    CPU->registers.pc++;
    setRegister(RT_D, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x12
static void ld_mde_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    writeBus(readRegister(RT_DE), CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x13
static void inc_de(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_DE) + 1;
    setRegister(RT_DE, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x14
static void inc_d(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_D) + 1;
    setRegister(RT_D, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 0, (CPU->fetchedData & 0x0F) == 0, -1);
}

//0x15
static void dec_d(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_D) - 1;
    setRegister(RT_D, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 1, (CPU->fetchedData & 0x0F) == 0x0F, -1);
}

//0x16
static void ld_d_u8(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    setRegister(RT_D, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x17
static void rla(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = CPU->registers.a;
    u8 cFlag = CPU_FLAG_C;
    u8 c = (CPU->fetchedData >> 7) & 1;
    CPU->registers.a = (CPU->fetchedData << 1) | cFlag;
    cpuSetFlags(CPU, 0, 0, 0, c);
}

//0x18
static void jr_i8(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    CPU->registers.pc++;
    int8_t relative = (int8_t)(CPU->fetchedData & 0xFF);
    u16 address = CPU->registers.pc + relative;
    waitForCPUCycle(1);
    CPU->registers.pc = address;
    waitForCPUCycle(1);
}

//0x19
static void add_hl_de(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_DE);
    u32 value = readRegister(RT_HL) + CPU->fetchedData;
    int h = ((readRegister(RT_HL) & 0xFFF) + (CPU->fetchedData & 0xFFF)) >= 0x1000;
    u32 i = ((u32)readRegister(RT_HL)) + ((u32)CPU->fetchedData);
    int c = i >= 0x10000;
    setRegister(RT_HL, value & 0xFFFF);
    cpuSetFlags(CPU, -1, 0, h, c);
    waitForCPUCycle(1);
}

//0x1A
static void ld_a_mde(cpuContext *CPU){
    waitForCPUCycle(1);
    setRegister(RT_A, readBus(readRegister(RT_DE)));
    waitForCPUCycle(1);
}

//0x1B
static void dec_de(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_DE) - 1;
    setRegister(RT_DE, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 1, (CPU->fetchedData & 0x0F) == 0x0F, -1);
    waitForCPUCycle(1);
}

//0x1C
static void inc_e(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_E) + 1;
    setRegister(RT_E, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 0, (CPU->fetchedData & 0x0F) == 0, -1);
}

//0x1D
static void dec_e(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_E) - 1;
    setRegister(RT_E, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 1, (CPU->fetchedData & 0x0F) == 0x0F, -1);
}

//0x1E
static void ld_e_u8(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    setRegister(RT_E, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x1F
static void rra(cpuContext *CPU){
    waitForCPUCycle(1);
    u8 carryFlag = CPU_FLAG_C;
    u8 newCarryFlag = CPU->registers.a & 1;
    CPU->registers.a >>= 1;
    CPU->registers.a |= (carryFlag << 7);
    cpuSetFlags(CPU, 0, 0, 0, newCarryFlag);
}

//0x20
static void jr_nz_i8(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    CPU->registers.pc++;
    int8_t relative = (int8_t)(CPU->fetchedData & 0xFF);
    u16 address = CPU->registers.pc + relative;
    waitForCPUCycle(1);
    if(!CPU_FLAG_Z){
        CPU->registers.pc = address;
        waitForCPUCycle(1);
    }
}

//0x21
static void ld_hl_u16(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    CPU->registers.pc++;
    setRegister(RT_L, CPU->fetchedData);
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    CPU->registers.pc++;
    setRegister(RT_H, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x22
static void ld_mhli_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    setRegister(RT_HL, readRegister(RT_HL) + 1);
    writeBus(readRegister(RT_HL), CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x23
static void inc_hl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_HL) + 1;
    setRegister(RT_HL, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x24
static void inc_h(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_H) + 1;
    setRegister(RT_H, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 0, (CPU->fetchedData & 0x0F) == 0, -1);
}

//0x25
static void dec_h(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_H) - 1;
    setRegister(RT_H, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 1, (CPU->fetchedData & 0x0F) == 0x0F, -1);
}

//0x26
static void ld_h_u8(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    setRegister(RT_H, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x27
static void daa(cpuContext *CPU){
    waitForCPUCycle(1);
    u8 value = 0;
    int cFlag = 0;
    if(CPU_FLAG_H || (!CPU_FLAG_N && (CPU->registers.a & 0xF) > 9)){
        value = 6;
    }
    if(CPU_FLAG_C || (!CPU_FLAG_N && CPU->registers.a > 0x99)){
        value |= 0x60;
        cFlag = 1;
    }
    CPU->registers.a += CPU_FLAG_N ? -value : value;
    cpuSetFlags(CPU, CPU->registers.a == 0, -1, 0, cFlag);
}

//0x28
static void jr_z_i8(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    CPU->registers.pc++;
    int8_t relative = (int8_t)(CPU->fetchedData & 0xFF);
    u16 address = CPU->registers.pc + relative;
    waitForCPUCycle(1);
    if(CPU_FLAG_Z){
        CPU->registers.pc = address;
        waitForCPUCycle(1);
    }
}

//0x29
static void add_hl_hl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_HL);
    u32 value = readRegister(RT_HL) + CPU->fetchedData;
    int h = ((readRegister(RT_HL) & 0xFFF) + (CPU->fetchedData & 0xFFF)) >= 0x1000;
    u32 i = ((u32)readRegister(RT_HL)) + ((u32)CPU->fetchedData);
    int c = i >= 0x10000;
    setRegister(RT_HL, value & 0xFFFF);
    cpuSetFlags(CPU, -1, 0, h, c);
    waitForCPUCycle(1);
}

//0x2A
static void ld_a_mhli(cpuContext *CPU){
    waitForCPUCycle(1);
    setRegister(RT_HL, readRegister(RT_HL) + 1);
    setRegister(RT_A, readBus(readRegister(RT_HL)));
    waitForCPUCycle(1);
}

//0x2B
static void dec_hl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_HL) - 1;
    setRegister(RT_HL, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 1, (CPU->fetchedData & 0x0F) == 0x0F, -1);
    waitForCPUCycle(1);
}

//0x2C
static void inc_l(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_L) + 1;
    setRegister(RT_L, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 0, (CPU->fetchedData & 0x0F) == 0, -1);
}

//0x2D
static void dec_l(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_L) - 1;
    setRegister(RT_L, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 1, (CPU->fetchedData & 0x0F) == 0x0F, -1);
}

//0x2E
static void ld_l_u8(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    setRegister(RT_L, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x2F
static void cpl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->registers.a = ~CPU->registers.a;
    cpuSetFlags(CPU, -1, 1, 1, -1);
}

//0x30
static void jr_nc_i8(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    CPU->registers.pc++;
    int8_t relative = (int8_t)(CPU->fetchedData & 0xFF);
    u16 address = CPU->registers.pc + relative;
    waitForCPUCycle(1);
    if(!CPU_FLAG_C){
        CPU->registers.pc = address;
        waitForCPUCycle(1);
    }
}

//0x31
static void ld_sp_u16(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    CPU->registers.pc++;
    waitForCPUCycle(1);
    CPU->fetchedData = (readBus(CPU->registers.pc) << 8 & 0xFF00) | CPU->fetchedData & 0xFF;
    CPU->registers.pc++;
    setRegister(RT_SP, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x32
static void ld_mhld_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    setRegister(RT_HL, readRegister(RT_HL) - 1);
    writeBus(readRegister(RT_HL), CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x33
static void inc_sp(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_SP) + 1;
    setRegister(RT_SP, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x34
static void inc_mhl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(readRegister(RT_HL)) + 1;
    CPU->fetchedData &= 0xFF;
    waitForCPUCycle(1);
    writeBus(readRegister(RT_HL), CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 0, (CPU->fetchedData & 0x0F) == 0, -1);
    waitForCPUCycle(1);
}

//0x35
static void dec_mhl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(readRegister(RT_HL)) - 1;
    CPU->fetchedData &= 0xFF;
    waitForCPUCycle(1);
    writeBus(readRegister(RT_HL), CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 1, (CPU->fetchedData & 0x0F) == 0x0F, -1);
    waitForCPUCycle(1);
}

//0x36
static void ld_mhl_u8(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    waitForCPUCycle(1);
    setRegister(RT_HL, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x37
static void scf(cpuContext *CPU){
    waitForCPUCycle(1);
    cpuSetFlags(CPU, -1, 0, 0, 1);
}

//0x38
static void jr_c_i8(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    CPU->registers.pc++;
    int8_t relative = (int8_t)(CPU->fetchedData & 0xFF);
    u16 address = CPU->registers.pc + relative;
    waitForCPUCycle(1);
    if(CPU_FLAG_C){
        CPU->registers.pc = address;
        waitForCPUCycle(1);
    }
}

//0x39
static void add_hl_sp(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_SP);
    u32 value = readRegister(RT_HL) + CPU->fetchedData;
    int h = ((readRegister(RT_HL) & 0xFFF) + (CPU->fetchedData & 0xFFF)) >= 0x1000;
    u32 i = ((u32)readRegister(RT_HL)) + ((u32)CPU->fetchedData);
    int c = i >= 0x10000;
    setRegister(RT_HL, value & 0xFFFF);
    cpuSetFlags(CPU, -1, 0, h, c);
    waitForCPUCycle(1);
}

//0x3A
static void ld_a_mhld(cpuContext *CPU){
    waitForCPUCycle(1);
    setRegister(RT_HL, readRegister(RT_HL) - 1);
    setRegister(RT_A, readBus(readRegister(RT_HL)));
    waitForCPUCycle(1);
}

//0x3B
static void dec_sp(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_SP) - 1;
    setRegister(RT_SP, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 1, (CPU->fetchedData & 0x0F) == 0x0F, -1);
    waitForCPUCycle(1);
}

//0x3C
static void inc_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A) + 1;
    setRegister(RT_A, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 0, (CPU->fetchedData & 0x0F) == 0, -1);
}

//0x3D
static void dec_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A) - 1;
    setRegister(RT_A, CPU->fetchedData);
    cpuSetFlags(CPU, CPU->fetchedData == 0, 1, (CPU->fetchedData & 0x0F) == 0x0F, -1);
}

//0x3E
static void ld_a_u8(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(CPU->registers.pc);
    setRegister(RT_A, CPU->fetchedData);
    waitForCPUCycle(1);
}

//0x3F
static void ccf(cpuContext *CPU){
    waitForCPUCycle(1);
    cpuSetFlags(CPU, -1, 0, 0, CPU_FLAG_C ^ 1);
}