#include <stdio.h>
#include <GameBoyEmulator.h>
#include <GameCartridge.h>
#include <Cpu.h>
#include <Ppu.h>
#include <SDL2/SDL.h>
#include <Ui.h>
#include <pthread.h>
#include <unistd.h>
#include <Timer.h>
#include <Dma.h>
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

void *runCpu(void * p){
    initCpu();
    initTimer();
    
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
    }
    return 0;
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

    //SDL_Init(SDL_INIT_VIDEO);
    //printf("INFO: SDL INIT\n");

    initUi();
    
    pthread_t thread1;

    if(pthread_create(&thread1, NULL, runCpu, NULL)){
        printf("\tERR: FAILED TO START THREAD\n");
        return -1;
    }
    u32 previousFrame = 0;
    while(!emulator.killEmu){
        delay(1);
        if(previousFrame != getPpuContext()->currentFrame){
            updateUi();
            previousFrame = getPpuContext()->currentFrame;
        }
        
        handleEventsUi();
    }
    return 0;
}

void waitForCPUCycle(int cycles){
    for(int i = 0; i < cycles; i++){
        for(int n = 0; n < 4; n++){
            emulator.ticks++;
            stepTimer();
            stepPpu();
        }
        stepDma();
    }
}