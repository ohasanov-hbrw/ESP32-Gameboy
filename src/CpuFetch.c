#include <Cpu.h>
#include <Bus.h>
#include <GameBoyEmulator.h>

extern cpuContext CPU;

void fetchData(){
    CPU.memoryDestination = 0;
    CPU.destinationIsMemory = false;

    switch(CPU.currentInstruction->mode){
        case AM_IMP:{
            return;
        }
        case AM_R:{
            CPU.fetchedData = readRegister(CPU.currentInstruction->reg1);
            return;
        }
        case AM_R_R:{
            CPU.fetchedData = readRegister(CPU.currentInstruction->reg2);
            return;
        }
        case AM_R_D8:{
            CPU.fetchedData = readBus(CPU.registers.pc);
            waitForCPUCycle(1);
            CPU.registers.pc++;
            return;
        }
        case AM_D16:{
            u16 low = readBus(CPU.registers.pc);
            waitForCPUCycle(1);
            u16 high = readBus(CPU.registers.pc + 1);
            waitForCPUCycle(1);
            CPU.fetchedData = low | (high << 8);
            CPU.registers.pc += 2;
            return;
        }
        case AM_R_D16:{
            u16 low = readBus(CPU.registers.pc);
            waitForCPUCycle(1);
            u16 high = readBus(CPU.registers.pc + 1);
            waitForCPUCycle(1);
            CPU.fetchedData = low | (high << 8);
            CPU.registers.pc += 2;
            return;
        }
        case AM_MR_R:{
            CPU.fetchedData = readRegister(CPU.currentInstruction->reg2);
            CPU.memoryDestination = readRegister(CPU.currentInstruction->reg1);
            CPU.destinationIsMemory = true;
            if(CPU.currentInstruction->reg1 == RT_C){
                CPU.memoryDestination |= 0xFF00;
            }
            return;
        }
        case AM_R_MR:{
            u16 address = readRegister(CPU.currentInstruction->reg2);
            if(CPU.currentInstruction->reg2 == RT_C){
                address |= 0xFF00;
            }
            CPU.fetchedData = readBus(address);
            waitForCPUCycle(1);
            return;
        }
        case AM_R_HLI:{
            CPU.fetchedData = readBus(readRegister(CPU.currentInstruction->reg2));
            waitForCPUCycle(1);
            setRegister(RT_HL, readRegister(RT_HL) + 1);
            return;
        }
        case AM_R_HLD:{
            CPU.fetchedData = readBus(readRegister(CPU.currentInstruction->reg2));
            waitForCPUCycle(1);
            setRegister(RT_HL, readRegister(RT_HL) - 1);
            return;
        }
        case AM_HLI_R:{
            CPU.fetchedData = readRegister(CPU.currentInstruction->reg2);
            CPU.memoryDestination = readRegister(CPU.currentInstruction->reg1);
            CPU.destinationIsMemory = true;
            setRegister(RT_HL, readRegister(RT_HL) + 1);
            return;
        }
        case AM_HLD_R:{
            CPU.fetchedData = readRegister(CPU.currentInstruction->reg2);
            CPU.memoryDestination = readRegister(CPU.currentInstruction->reg1);
            CPU.destinationIsMemory = true;
            setRegister(RT_HL, readRegister(RT_HL) - 1);
            return;
        }
        case AM_R_A8:{
            CPU.fetchedData = readBus(CPU.registers.pc);
            waitForCPUCycle(1);
            CPU.registers.pc++;
            return;
        }
        case AM_A8_R:{
            CPU.memoryDestination = readBus(CPU.registers.pc) | 0xFF00;
            CPU.destinationIsMemory = true;
            waitForCPUCycle(1);
            CPU.registers.pc++;
            return;
        }
        case AM_HL_SPR:{
            CPU.fetchedData = readBus(CPU.registers.pc);
            waitForCPUCycle(1);
            CPU.registers.pc++;
            return;
        }
        case AM_D8:{
            CPU.fetchedData = readBus(CPU.registers.pc);
            waitForCPUCycle(1);
            CPU.registers.pc++;
            return;
        }
        case AM_D16_R:{
            u16 low = readBus(CPU.registers.pc);
            waitForCPUCycle(1);
            u16 high = readBus(CPU.registers.pc + 1);
            waitForCPUCycle(1);
            CPU.memoryDestination = low | (high << 8);
            CPU.destinationIsMemory = true;
            CPU.registers.pc += 2;
            CPU.fetchedData = readRegister(CPU.currentInstruction->reg2);
            return;
        }
        case AM_MR_D8:{
            CPU.fetchedData = readBus(CPU.registers.pc);
            waitForCPUCycle(1);
            CPU.registers.pc++;
            CPU.memoryDestination = readRegister(CPU.currentInstruction->reg1);
            CPU.destinationIsMemory = true;
            return;
        }
        case AM_MR:{
            CPU.memoryDestination = readRegister(CPU.currentInstruction->reg1);
            CPU.destinationIsMemory = true;
            CPU.fetchedData = readBus(readRegister(CPU.currentInstruction->reg1));
            waitForCPUCycle(1);
            return;
        }
        case AM_A16_R:{
            u16 low = readBus(CPU.registers.pc);
            waitForCPUCycle(1);
            u16 high = readBus(CPU.registers.pc + 1);
            waitForCPUCycle(1);
            CPU.memoryDestination = low | (high << 8);
            CPU.destinationIsMemory = true;
            CPU.registers.pc += 2;
            CPU.fetchedData = readRegister(CPU.currentInstruction->reg2);
            return;
        }
        case AM_R_A16:{
            u16 low = readBus(CPU.registers.pc);
            waitForCPUCycle(1);
            u16 high = readBus(CPU.registers.pc + 1);
            waitForCPUCycle(1);
            u16 address = low | (high << 8);
            CPU.registers.pc += 2;
            CPU.fetchedData = readBus(address);
            waitForCPUCycle(1);
            return;
        }
        default:{
            printf("\tERR: UNKN ADDR MODE: %d 0x%02X\n", CPU.currentInstruction->mode, CPU.currentOpcode);
            exit(-31);
            return;
        }
    }
} 