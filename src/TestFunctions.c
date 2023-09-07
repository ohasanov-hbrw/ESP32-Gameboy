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

//0x40
static void ld_b_b(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_B);
    setRegister(RT_B, CPU->fetchedData);
}

//0x41
static void ld_b_c(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_C);
    setRegister(RT_B, CPU->fetchedData);
}

//0x42
static void ld_b_d(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_D);
    setRegister(RT_B, CPU->fetchedData);
}

//0x43
static void ld_b_e(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_E);
    setRegister(RT_B, CPU->fetchedData);
}

//0x44
static void ld_b_h(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_H);
    setRegister(RT_B, CPU->fetchedData);
}

//0x45
static void ld_b_l(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_L);
    setRegister(RT_B, CPU->fetchedData);
}

//0x46
static void ld_b_mhl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(readRegister(RT_HL));
    waitForCPUCycle(1);
    setRegister(RT_B, CPU->fetchedData);
}

//0x47
static void ld_b_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    setRegister(RT_B, CPU->fetchedData);
}

//0x48
static void ld_c_b(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_B);
    setRegister(RT_C, CPU->fetchedData);
}

//0x49
static void ld_c_c(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_C);
    setRegister(RT_C, CPU->fetchedData);
}

//0x4A
static void ld_c_d(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_D);
    setRegister(RT_C, CPU->fetchedData);
}

//0x4B
static void ld_c_e(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_E);
    setRegister(RT_C, CPU->fetchedData);
}

//0x4C
static void ld_c_h(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_H);
    setRegister(RT_C, CPU->fetchedData);
}

//0x4D
static void ld_c_l(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_L);
    setRegister(RT_C, CPU->fetchedData);
}

//0x4E
static void ld_c_mhl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(readRegister(RT_HL));
    waitForCPUCycle(1);
    setRegister(RT_C, CPU->fetchedData);
}

//0x4F
static void ld_c_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    setRegister(RT_C, CPU->fetchedData);
}

//0x50
static void ld_d_b(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_B);
    setRegister(RT_D, CPU->fetchedData);
}

//0x51
static void ld_d_c(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_C);
    setRegister(RT_D, CPU->fetchedData);
}

//0x52
static void ld_d_d(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_D);
    setRegister(RT_D, CPU->fetchedData);
}

//0x53
static void ld_d_e(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_E);
    setRegister(RT_D, CPU->fetchedData);
}

//0x54
static void ld_d_h(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_H);
    setRegister(RT_D, CPU->fetchedData);
}

//0x55
static void ld_d_l(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_L);
    setRegister(RT_D, CPU->fetchedData);
}

//0x56
static void ld_d_mhl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(readRegister(RT_HL));
    waitForCPUCycle(1);
    setRegister(RT_D, CPU->fetchedData);
}

//0x57
static void ld_d_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    setRegister(RT_D, CPU->fetchedData);
}

//0x58
static void ld_e_b(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_B);
    setRegister(RT_E, CPU->fetchedData);
}

//0x59
static void ld_e_c(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_C);
    setRegister(RT_E, CPU->fetchedData);
}

//0x5A
static void ld_e_d(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_D);
    setRegister(RT_E, CPU->fetchedData);
}

//0x5B
static void ld_e_e(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_E);
    setRegister(RT_E, CPU->fetchedData);
}

//0x5C
static void ld_e_h(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_H);
    setRegister(RT_E, CPU->fetchedData);
}

//0x5D
static void ld_e_l(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_L);
    setRegister(RT_E, CPU->fetchedData);
}

//0x5E
static void ld_e_mhl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(readRegister(RT_HL));
    waitForCPUCycle(1);
    setRegister(RT_E, CPU->fetchedData);
}

//0x5F
static void ld_e_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    setRegister(RT_E, CPU->fetchedData);
}

//0x60
static void ld_h_b(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_B);
    setRegister(RT_H, CPU->fetchedData);
}

//0x61
static void ld_h_c(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_C);
    setRegister(RT_H, CPU->fetchedData);
}

//0x62
static void ld_h_d(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_D);
    setRegister(RT_H, CPU->fetchedData);
}

//0x63
static void ld_h_e(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_E);
    setRegister(RT_H, CPU->fetchedData);
}

//0x64
static void ld_h_h(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_H);
    setRegister(RT_H, CPU->fetchedData);
}

//0x65
static void ld_h_l(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_L);
    setRegister(RT_H, CPU->fetchedData);
}

//0x66
static void ld_h_mhl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(readRegister(RT_HL));
    waitForCPUCycle(1);
    setRegister(RT_H, CPU->fetchedData);
}

//0x67
static void ld_h_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    setRegister(RT_H, CPU->fetchedData);
}

//0x68
static void ld_l_b(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_B);
    setRegister(RT_L, CPU->fetchedData);
}

//0x69
static void ld_l_c(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_C);
    setRegister(RT_L, CPU->fetchedData);
}

//0x6A
static void ld_l_d(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_D);
    setRegister(RT_L, CPU->fetchedData);
}

//0x6B
static void ld_l_e(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_E);
    setRegister(RT_L, CPU->fetchedData);
}

//0x6C
static void ld_l_h(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_H);
    setRegister(RT_L, CPU->fetchedData);
}

//0x6D
static void ld_l_l(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_L);
    setRegister(RT_L, CPU->fetchedData);
}

//0x6E
static void ld_l_mhl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(readRegister(RT_HL));
    waitForCPUCycle(1);
    setRegister(RT_L, CPU->fetchedData);
}

//0x6F
static void ld_l_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    setRegister(RT_L, CPU->fetchedData);
}

//0x70
static void ld_mhl_b(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_B);
    waitForCPUCycle(1);
    writeBus(readRegister(RT_HL), CPU->fetchedData);
}

//0x71
static void ld_mhl_c(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_C);
    waitForCPUCycle(1);
    writeBus(readRegister(RT_HL), CPU->fetchedData);
}

//0x72
static void ld_mhl_d(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_D);
    waitForCPUCycle(1);
    writeBus(readRegister(RT_HL), CPU->fetchedData);
}

//0x73
static void ld_mhl_e(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_E);
    waitForCPUCycle(1);
    writeBus(readRegister(RT_HL), CPU->fetchedData);
}

//0x74
static void ld_mhl_h(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_H);
    waitForCPUCycle(1);
    writeBus(readRegister(RT_HL), CPU->fetchedData);
}

//0x75
static void ld_mhl_l(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_L);
    waitForCPUCycle(1);
    writeBus(readRegister(RT_HL), CPU->fetchedData);
}

//0x76
static void halt(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->halted = true;
}

//0x77
static void ld_mhl_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    waitForCPUCycle(1);
    writeBus(readRegister(RT_HL), CPU->fetchedData);
}

//0x78
static void ld_a_b(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_B);
    setRegister(RT_A, CPU->fetchedData);
}

//0x79
static void ld_a_c(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_C);
    setRegister(RT_A, CPU->fetchedData);
}

//0x7A
static void ld_a_d(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_D);
    setRegister(RT_A, CPU->fetchedData);
}

//0x7B
static void ld_a_e(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_E);
    setRegister(RT_A, CPU->fetchedData);
}

//0x7C
static void ld_a_h(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_H);
    setRegister(RT_A, CPU->fetchedData);
}

//0x7D
static void ld_a_l(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_L);
    setRegister(RT_A, CPU->fetchedData);
}

//0x7E
static void ld_a_mhl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(readRegister(RT_HL));
    waitForCPUCycle(1);
    setRegister(RT_A, CPU->fetchedData);
}

//0x7F
static void ld_a_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    setRegister(RT_A, CPU->fetchedData);
}

//0x80
static void add_a_b(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_B);
    u32 value = readRegister(RT_A) + CPU->fetchedData;
    int z = (value & 0xFF) == 0;
    int h = (readRegister(RT_A) & 0xF) + (CPU->fetchedData & 0xF) >= 0x10;
    int c = (int)(readRegister(RT_A) & 0xFF) + (int)(CPU->fetchedData & 0xFF) >= 0x100;
    setRegister(RT_A, value & 0xFFFF);
    cpuSetFlags(CPU, z, 0, h, c);
}

//0x81
static void add_a_c(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_C);
    u32 value = readRegister(RT_A) + CPU->fetchedData;
    int z = (value & 0xFF) == 0;
    int h = (readRegister(RT_A) & 0xF) + (CPU->fetchedData & 0xF) >= 0x10;
    int c = (int)(readRegister(RT_A) & 0xFF) + (int)(CPU->fetchedData & 0xFF) >= 0x100;
    setRegister(RT_A, value & 0xFFFF);
    cpuSetFlags(CPU, z, 0, h, c);
}

//0x82
static void add_a_d(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_D);
    u32 value = readRegister(RT_A) + CPU->fetchedData;
    int z = (value & 0xFF) == 0;
    int h = (readRegister(RT_A) & 0xF) + (CPU->fetchedData & 0xF) >= 0x10;
    int c = (int)(readRegister(RT_A) & 0xFF) + (int)(CPU->fetchedData & 0xFF) >= 0x100;
    setRegister(RT_A, value & 0xFFFF);
    cpuSetFlags(CPU, z, 0, h, c);
}

//0x83
static void add_a_e(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_E);
    u32 value = readRegister(RT_A) + CPU->fetchedData;
    int z = (value & 0xFF) == 0;
    int h = (readRegister(RT_A) & 0xF) + (CPU->fetchedData & 0xF) >= 0x10;
    int c = (int)(readRegister(RT_A) & 0xFF) + (int)(CPU->fetchedData & 0xFF) >= 0x100;
    setRegister(RT_A, value & 0xFFFF);
    cpuSetFlags(CPU, z, 0, h, c);
}

//0x84
static void add_a_h(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_H);
    u32 value = readRegister(RT_A) + CPU->fetchedData;
    int z = (value & 0xFF) == 0;
    int h = (readRegister(RT_A) & 0xF) + (CPU->fetchedData & 0xF) >= 0x10;
    int c = (int)(readRegister(RT_A) & 0xFF) + (int)(CPU->fetchedData & 0xFF) >= 0x100;
    setRegister(RT_A, value & 0xFFFF);
    cpuSetFlags(CPU, z, 0, h, c);
}

//0x85
static void add_a_l(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_L);
    u32 value = readRegister(RT_A) + CPU->fetchedData;
    int z = (value & 0xFF) == 0;
    int h = (readRegister(RT_A) & 0xF) + (CPU->fetchedData & 0xF) >= 0x10;
    int c = (int)(readRegister(RT_A) & 0xFF) + (int)(CPU->fetchedData & 0xFF) >= 0x100;
    setRegister(RT_A, value & 0xFFFF);
    cpuSetFlags(CPU, z, 0, h, c);
}

//0x86
static void add_a_mhl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(readRegister(RT_HL));
    waitForCPUCycle(1);
    u32 value = readRegister(RT_A) + CPU->fetchedData;
    int z = (value & 0xFF) == 0;
    int h = (readRegister(RT_A) & 0xF) + (CPU->fetchedData & 0xF) >= 0x10;
    int c = (int)(readRegister(RT_A) & 0xFF) + (int)(CPU->fetchedData & 0xFF) >= 0x100;
    setRegister(RT_A, value & 0xFFFF);
    cpuSetFlags(CPU, z, 0, h, c);
}

//0x87
static void add_a_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    u32 value = readRegister(RT_A) + CPU->fetchedData;
    int z = (value & 0xFF) == 0;
    int h = (readRegister(RT_A) & 0xF) + (CPU->fetchedData & 0xF) >= 0x10;
    int c = (int)(readRegister(RT_A) & 0xFF) + (int)(CPU->fetchedData & 0xFF) >= 0x100;
    setRegister(RT_A, value & 0xFFFF);
    cpuSetFlags(CPU, z, 0, h, c);
}

//0x88
static void adc_a_b(cpuContext *CPU){
    waitForCPUCycle(1);
    u16 u = readRegister(RT_B);
    u16 a = readRegister(RT_A);
    u16 c = CPU_FLAG_C;
    setRegister(RT_A, (a + u + c) & 0xFF);
    cpuSetFlags(CPU, readRegister(RT_A) == 0, 0, (a & 0xF) + (u & 0xF) + c > 0xF, a + u + c > 0xFF);
}

//0x89
static void adc_a_c(cpuContext *CPU){
    waitForCPUCycle(1);
    u16 u = readRegister(RT_C);
    u16 a = readRegister(RT_A);
    u16 c = CPU_FLAG_C;
    setRegister(RT_A, (a + u + c) & 0xFF);
    cpuSetFlags(CPU, readRegister(RT_A) == 0, 0, (a & 0xF) + (u & 0xF) + c > 0xF, a + u + c > 0xFF);
}

//0x8A
static void adc_a_d(cpuContext *CPU){
    waitForCPUCycle(1);
    u16 u = readRegister(RT_D);
    u16 a = readRegister(RT_A);
    u16 c = CPU_FLAG_C;
    setRegister(RT_A, (a + u + c) & 0xFF);
    cpuSetFlags(CPU, readRegister(RT_A) == 0, 0, (a & 0xF) + (u & 0xF) + c > 0xF, a + u + c > 0xFF);
}

//0x8B
static void adc_a_e(cpuContext *CPU){
    waitForCPUCycle(1);
    u16 u = readRegister(RT_E);
    u16 a = readRegister(RT_A);
    u16 c = CPU_FLAG_C;
    setRegister(RT_A, (a + u + c) & 0xFF);
    cpuSetFlags(CPU, readRegister(RT_A) == 0, 0, (a & 0xF) + (u & 0xF) + c > 0xF, a + u + c > 0xFF);
}

//0x8C
static void adc_a_h(cpuContext *CPU){
    waitForCPUCycle(1);
    u16 u = readRegister(RT_H);
    u16 a = readRegister(RT_A);
    u16 c = CPU_FLAG_C;
    setRegister(RT_A, (a + u + c) & 0xFF);
    cpuSetFlags(CPU, readRegister(RT_A) == 0, 0, (a & 0xF) + (u & 0xF) + c > 0xF, a + u + c > 0xFF);
}

//0x8D
static void adc_a_l(cpuContext *CPU){
    waitForCPUCycle(1);
    u16 u = readRegister(RT_L);
    u16 a = readRegister(RT_A);
    u16 c = CPU_FLAG_C;
    setRegister(RT_A, (a + u + c) & 0xFF);
    cpuSetFlags(CPU, readRegister(RT_A) == 0, 0, (a & 0xF) + (u & 0xF) + c > 0xF, a + u + c > 0xFF);
}

//0x8E
static void adc_a_mhl(cpuContext *CPU){
    waitForCPUCycle(1);
    u16 u = readBus(readRegister(RT_HL));
    waitForCPUCycle(1);
    u16 a = readRegister(RT_A);
    u16 c = CPU_FLAG_C;
    setRegister(RT_A, (a + u + c) & 0xFF);
    cpuSetFlags(CPU, readRegister(RT_A) == 0, 0, (a & 0xF) + (u & 0xF) + c > 0xF, a + u + c > 0xFF);
}

//0x8F
static void adc_a_a(cpuContext *CPU){
    waitForCPUCycle(1);
    u16 u = readRegister(RT_A);
    u16 a = readRegister(RT_A);
    u16 c = CPU_FLAG_C;
    setRegister(RT_A, (a + u + c) & 0xFF);
    cpuSetFlags(CPU, readRegister(RT_A) == 0, 0, (a & 0xF) + (u & 0xF) + c > 0xF, a + u + c > 0xFF);
}

//0x90
static void sub_a_b(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_B);
    u16 value = readRegister(RT_A) - CPU->fetchedData;
    int z = value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) < 0;
    setRegister(RT_A, value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x91
static void sub_a_c(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_C);
    u16 value = readRegister(RT_A) - CPU->fetchedData;
    int z = value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) < 0;
    setRegister(RT_A, value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x92
static void sub_a_d(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_D);
    u16 value = readRegister(RT_A) - CPU->fetchedData;
    int z = value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) < 0;
    setRegister(RT_A, value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x93
static void sub_a_e(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_E);
    u16 value = readRegister(RT_A) - CPU->fetchedData;
    int z = value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) < 0;
    setRegister(RT_A, value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x94
static void sub_a_h(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_H);
    u16 value = readRegister(RT_A) - CPU->fetchedData;
    int z = value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) < 0;
    setRegister(RT_A, value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x95
static void sub_a_l(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_L);
    u16 value = readRegister(RT_A) - CPU->fetchedData;
    int z = value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) < 0;
    setRegister(RT_A, value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x96
static void sub_a_mhl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(readRegister(RT_HL));
    waitForCPUCycle(1);
    u16 value = readRegister(RT_A) - CPU->fetchedData;
    int z = value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) < 0;
    setRegister(RT_A, value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x97
static void sub_a_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    u16 value = readRegister(RT_A) - CPU->fetchedData;
    int z = value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) < 0;
    setRegister(RT_A, value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x98
static void sbc_a_b(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_B);
    u8 value = CPU->fetchedData + CPU_FLAG_C;
    int z = readRegister(RT_A) - value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) - ((int)CPU_FLAG_C) < 0;
    setRegister(RT_A, readRegister(RT_A) - value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x99
static void sbc_a_c(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_C);
    u8 value = CPU->fetchedData + CPU_FLAG_C;
    int z = readRegister(RT_A) - value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) - ((int)CPU_FLAG_C) < 0;
    setRegister(RT_A, readRegister(RT_A) - value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x9A
static void sbc_a_d(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_D);
    u8 value = CPU->fetchedData + CPU_FLAG_C;
    int z = readRegister(RT_A) - value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) - ((int)CPU_FLAG_C) < 0;
    setRegister(RT_A, readRegister(RT_A) - value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x9B
static void sbc_a_e(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_E);
    u8 value = CPU->fetchedData + CPU_FLAG_C;
    int z = readRegister(RT_A) - value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) - ((int)CPU_FLAG_C) < 0;
    setRegister(RT_A, readRegister(RT_A) - value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x9C
static void sbc_a_h(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_H);
    u8 value = CPU->fetchedData + CPU_FLAG_C;
    int z = readRegister(RT_A) - value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) - ((int)CPU_FLAG_C) < 0;
    setRegister(RT_A, readRegister(RT_A) - value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x9D
static void sbc_a_l(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_L);
    u8 value = CPU->fetchedData + CPU_FLAG_C;
    int z = readRegister(RT_A) - value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) - ((int)CPU_FLAG_C) < 0;
    setRegister(RT_A, readRegister(RT_A) - value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x9E
static void sbc_a_mhl(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readBus(readRegister(RT_HL));
    waitForCPUCycle(1);
    u8 value = CPU->fetchedData + CPU_FLAG_C;
    int z = readRegister(RT_A) - value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) - ((int)CPU_FLAG_C) < 0;
    setRegister(RT_A, readRegister(RT_A) - value);
    cpuSetFlags(CPU, z, 1, h, c);
}

//0x9F
static void sbc_a_a(cpuContext *CPU){
    waitForCPUCycle(1);
    CPU->fetchedData = readRegister(RT_A);
    u8 value = CPU->fetchedData + CPU_FLAG_C;
    int z = readRegister(RT_A) - value == 0;
    int h = ((int)readRegister(RT_A) & 0xF) - ((int)CPU->fetchedData & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)readRegister(RT_A)) - ((int)CPU->fetchedData) - ((int)CPU_FLAG_C) < 0;
    setRegister(RT_A, readRegister(RT_A) - value);
    cpuSetFlags(CPU, z, 1, h, c);
}