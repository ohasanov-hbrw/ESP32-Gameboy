#pragma once

#include <Utils.h>

typedef struct{
    bool paused;
    bool running;
    u64 ticks;
}EmulatorContext;

int runEmulator(int argc, char **argv);

EmulatorContext *GetEmulatorContext();

void waitForCPUCycle(int cycles);