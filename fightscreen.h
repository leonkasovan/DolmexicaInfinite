#pragma once

#include <stdio.h>
#include <prism/wrapper.h>

using namespace prism;

void startFightScreen(void(*tWinCB)(), void(*tLoseCB)() = NULL);
void reloadFightScreen();
void stopFightScreenWin();
void stopFightScreenLose();
void stopFightScreenToFixedScreen(Screen* tNextScreen);

Screen* getDreamFightScreenForTesting();