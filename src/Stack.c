#include <Stack.h>
#include <Cpu.h>
#include <Bus.h>

void pushToStack(u8 value){
    getRegisters()->sp--;
    //printf("push 0x%04X 0x%02X\n",getRegisters()->sp, value);
    writeBus(getRegisters()->sp, value);
}

void push16ToStack(u16 value){
    pushToStack((value >> 8) & 0xFF);
    pushToStack(value & 0xFF);
}

u8 popFromStack(){
    //printf("pop 0x%04X 0x%02X\n",getRegisters()->sp, readBus(getRegisters()->sp));
    return readBus(getRegisters()->sp++);
}

u16 pop16FromStack(){
    u16 low = popFromStack();
    u16 high = popFromStack();
    return (high << 8) | low;
}