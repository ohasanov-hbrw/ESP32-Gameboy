#include <stdio.h>
#include <GameBoyEmulator.h>
#include <GameCartridge.h>
#include <Cpu.h>
#include <SDL2/SDL.h>

/* 
  Emulator components:

  |Cart|
  |CPU|
  |Address Bus|
  |PPU|
  |Timer|

*/

static EmulatorContext emulator;

EmulatorContext *GetEmulatorContext(){
    return &emulator;
}

void delay(u32 ms){
    SDL_Delay(ms);
}

int runEmulator(int argc, char **argv){
    if(argc < 2){
        printf("Usage: emu <rom_file>\n");
        return -1;
    }

    if(!loadCartridge(argv[1])){
        printf("Failed to load ROM file: %s\n", argv[1]);
        return -2;
    }

    printf("INFO: Cart loaded..\n");

    SDL_Init(SDL_INIT_VIDEO);
    printf("INFO: SDL INIT\n");

    initCpu();
    
    emulator.running = true;
    emulator.paused = false;
    emulator.ticks = 0;

    while(emulator.running){
        if(emulator.paused){
            delay(10);
            continue;
        }

        if(!stepCpu()){
            printf("CPU Stopped\n");
            return -3;
        }

        emulator.ticks++;
    }

    return 0;
}

void waitForCPUCycle(int cycles){

}