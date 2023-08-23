#include <Stack.h>
#include <Cpu.h>
#include <Bus.h>

void pushToStack(u8 value){
    getRegisters()->sp--;
    writeBus(getRegisters()->sp, value);
}

void push16ToStack(u16 value){
    pushToStack((value >> 8) & 0xFF);
    pushToStack(value & 0xFF);
}

u8 popFromStack(){
    readBus(getRegisters()->sp);
    getRegisters()->sp++;
}

u16 pop16FromStack(){
    u16 low = popFromStack();
    u16 high = popFromStack();
    return (high << 8) | low;
}