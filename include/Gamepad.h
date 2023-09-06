#pragma once

#include <Utils.h>

typedef struct{
    bool start;
    bool select;
    bool a;
    bool b;
    bool up;
    bool down;
    bool left;
    bool right;
}gamepadState;

typedef struct{
    bool buttonSelect;
    bool dirSelect;
    gamepadState controller;
}gamepadContext;

void initGamepad();
bool buttonSelectGamepad();
bool directionSelectGamepad();
void setSelectGamepad(u8 value);

gamepadState *getGamepadState();
u8 getGamePadOutput();