#include <Gamepad.h>
#include <string.h>


static gamepadContext Gamepad = {0};

bool buttonSelectGamepad(){
    return Gamepad.buttonSelect;
}

bool directionSelectGamepad(){
    return Gamepad.dirSelect;
}

void setSelectGamepad(u8 value){
    Gamepad.buttonSelect = value & 0x20;
    Gamepad.dirSelect = value & 0x10;
}

gamepadState *getGamepadState(){
    return &Gamepad.controller;
}

u8 getGamePadOutput(){
    u8 output = 0xCF;
    if(!buttonSelectGamepad()){
        if(getGamepadState()->start){
            output &= ~(1 << 3);
        } 
        if(getGamepadState()->select){
            output &= ~(1 << 2);
        } 
        if(getGamepadState()->a){
            output &= ~(1 << 0);
        } 
        if(getGamepadState()->b){
            output &= ~(1 << 1);
        }
    }
    if(!directionSelectGamepad()){
        if(getGamepadState()->left){
            output &= ~(1 << 1);
        } 
        if(getGamepadState()->right){
            output &= ~(1 << 0);
        } 
        if(getGamepadState()->up){
            output &= ~(1 << 2);
        } 
        if(getGamepadState()->down){
            output &= ~(1 << 3);
        }
    }
    return output;
}