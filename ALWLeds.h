/*
  ALWLeds.h - Library for flashing ALWLeds code.
  Created by David A. Mellis, November 2, 2007.
  Released into the public domain.
*/
#ifndef ALWLeds_h
#define ALWLeds_h

#include "Arduino.h"

#define MAX_BRIGHTNESS 3
// While we limited it to 27, there are 28 GPIO ports.
// A prototyping board used PE1, so we need this.
#define MAX_NUM_LEDS 28

#define COLOR(r, g, b) ( (uint32_t)(r) << 16 | (uint32_t)(g) << 8 | (uint32_t)(b) )
#define RED(color) ( (color) >> 16 & 0xFF )
#define GREEN(color) ( (color) >> 8 & 0xFF )
#define BLUE(color) ( (color) >> 0 & 0xFF )
#define COLOR_USE_MAX(color) ( max(max(RED(color), GREEN(color)), BLUE(color)) )
#define MAX_HUE 1535

/*
 * An enum would be best here, but its variable width will be unknown when
 * we try to read it from program memory space.
 */
// Color may not matter for a while, but it doesn't hurt to have it.
#define LED_TYPE_BLUE  1
#define LED_TYPE_WHITE 2
// Needed for wiring order. Prototype boards are RGB but production are GBR.
// 4 other orders are possible but do not currently exist.
#define LED_TYPE_GBR   3
#define LED_TYPE_RGB   4
// Annoying special-case for our prototyping board: skip between R and G.
#define LED_TYPE_RSGB 127

// Same as above: enum is unpredictable.
#define LED_FRAME_TRANS_LINEAR 1

typedef struct LEDData {
  uint8_t portIndex;
  uint8_t type;
} LEDData;
typedef union {
  LEDData data;
  uint16_t word;
} WordToLEDData;

typedef uint32_t Color;

typedef struct FrameData {
  float duration;
  float transTime;
  uint8_t transType;
  uint8_t numColors;
  const Color* colors;
} FrameData;
typedef struct FrameCollection {
  uint8_t numFrames;
  const FrameData* frames;
} FrameCollection;

typedef void (*delay_func_t)(unsigned long);

class ALWLeds
{
  public:
    static ALWLeds* singleton;
    // Note: this pointer's address is in the program-memory space.
    void init(const LEDData* data, byte logicalNumber);
    void clear();
    // useMax uses max(R, G, B) when setting monochrome LEDs
    void fill(Color color, bool useMax = false);
    void setLedColor(byte index, Color color, byte* dest = NULL, bool useMax = false);
    Color getLedColor(byte index);
    void updatePorts();
    FrameData loadFrame(const FrameData* f, byte* dest = NULL);
    void fadeFrames(const FrameData* from, const FrameData* to, delay_func_t delayFunction);

    // Self explanatory; return the new/current brightness.
    byte getBrightness();
    byte setBrightness(byte b);
    byte increaseBrightness();
    byte decreaseBrightness();
    static Color hsv2rgb(uint16_t h, byte s = 255, byte v = 255);
  private:
    const LEDData* ledData;
    byte numLogicalLeds;
    byte pwmValues[MAX_NUM_LEDS];
    byte updatePort(uint8_t portIndex, uint8_t numPins, uint16_t firstLed);
};

#endif
