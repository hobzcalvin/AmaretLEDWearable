/*
  ALWBase.h -
*/
#ifndef ALWBase_h
#define ALWBase_h

#include "Arduino.h"

#define NUM_BUTTONS 2
typedef void (*action_func_t)(void);
#define ENTER_BOOTLOADER_FUNCTION ((action_func_t)0x27fc) /* 0x4ff8 / 2 */

class ALWBase
{
  public:
    static ALWBase* singleton;
    action_func_t buttonClickFunctions[NUM_BUTTONS];
    action_func_t allButtonsFunction;

    ALWBase();
    void init();
    void sleep();
    void idle();
    void fixClockTiming();
    void buttonInterrupt(byte button);
  private:
    byte lastButton;
    unsigned long debounceTarget;
    bool inSleep;
};

#endif
