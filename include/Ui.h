#pragma once

#include <Utils.h>


static const int SCREEN_WIDTH = 320;
static const int SCREEN_HEIGHT = 288;

void initUi();
void delay(u32);
void CreateText(const char*);
void handleEventsUi();
void updateUi();