/*
  ALWLeds.cpp - Library for flashing ALWLeds code.
  Created by David A. Mellis, November 2, 2007.
  Released into the public domain.
*/

#include "Arduino.h"
#include "ALWLeds.h"
#include "ALWBase.h"
#include <util/delay.h>

#define PWM_CYCLE_TIME_uS 1000.0
#define PWM_ON_FRACTION 0.2
#define MAX_PWM_VALUE 63

// Can't be a private member of ALWLeds since the ISR needs access to it
// and we don't want to waste a function call.
// Initialize with all LEDs off.
byte ALWLeds_portMasks[5 * MAX_PWM_VALUE];

volatile static bool updatePortMasks = true;
volatile static uint16_t curFrame;
volatile static byte brightness = MAX_BRIGHTNESS;

void setPorts() {
  PORTA.OUT = ALWLeds_portMasks[curFrame++];
  PORTB.OUT = ALWLeds_portMasks[curFrame++];
  PORTC.OUT = ALWLeds_portMasks[curFrame++];
  PORTD.OUT = ALWLeds_portMasks[curFrame++];
  PORTE.OUT = ALWLeds_portMasks[curFrame++];
}
ISR(TCC0_OVF_vect) {
  if (!brightness) {
    return;
  }
  curFrame = 0;

#define SET_AND_DELAY(time) \
  setPorts(); \
  _delay_us(time * PWM_CYCLE_TIME_uS);

  // Big ugly block where we set each frame and delay a calculated amount
  // Formula: (i/MAX_PWM_VALUE)^2.5, then the delta between successive values.
  // The total of all these constants should be 1, so that PWM_CYCLE_TIME_uS
  // denotes the time it takes for all of these to run.
  SET_AND_DELAY(0.00003174304804);
  SET_AND_DELAY(0.000147822748156);
  SET_AND_DELAY(0.000315259351732);
  SET_AND_DELAY(0.000520952389344);
  SET_AND_DELAY(0.000758712793476);
  SET_AND_DELAY(0.001024663410061);
  SET_AND_DELAY(0.00131607259664);
  SET_AND_DELAY(0.001630879140806);
  SET_AND_DELAY(0.001967455195403);
  SET_AND_DELAY(0.002324472494516);
  SET_AND_DELAY(0.002700820217883);
  SET_AND_DELAY(0.003095551347621);
  SET_AND_DELAY(0.003507845927934);
  SET_AND_DELAY(0.003936984933008);
  SET_AND_DELAY(0.004382331098934);
  SET_AND_DELAY(0.004843314499145);
  SET_AND_DELAY(0.005319421451962);
  SET_AND_DELAY(0.005810185830838);
  SET_AND_DELAY(0.006315182146539);
  SET_AND_DELAY(0.00683401996189);
  SET_AND_DELAY(0.007366339326032);
  SET_AND_DELAY(0.007911807000617);
  SET_AND_DELAY(0.008470113309556);
  SET_AND_DELAY(0.009040969485753);
  SET_AND_DELAY(0.009624105418313);
  SET_AND_DELAY(0.010219267725684);
  SET_AND_DELAY(0.010826218096481);
  SET_AND_DELAY(0.011444731851991);
  SET_AND_DELAY(0.012074596693659);
  SET_AND_DELAY(0.012715611606027);
  SET_AND_DELAY(0.013367585891158);
  SET_AND_DELAY(0.014030338314958);
  SET_AND_DELAY(0.014703696349235);
  SET_AND_DELAY(0.015387495496137);
  SET_AND_DELAY(0.016081578683784);
  SET_AND_DELAY(0.016785795723735);
  SET_AND_DELAY(0.017500002822384);
  SET_AND_DELAY(0.018224062139584);
  SET_AND_DELAY(0.018957841388777);
  SET_AND_DELAY(0.019701213473754);
  SET_AND_DELAY(0.020454056157834);
  SET_AND_DELAY(0.021216251761832);
  SET_AND_DELAY(0.021987686887674);
  SET_AND_DELAY(0.022768252164931);
  SET_AND_DELAY(0.023557842017875);
  SET_AND_DELAY(0.024356354450971);
  SET_AND_DELAY(0.02516369085098);
  SET_AND_DELAY(0.025979755804035);
  SET_AND_DELAY(0.026804456926293);
  SET_AND_DELAY(0.027637704706855);
  SET_AND_DELAY(0.028479412361882);
  SET_AND_DELAY(0.029329495698852);
  SET_AND_DELAY(0.030187872990113);
  SET_AND_DELAY(0.031054464854895);
  SET_AND_DELAY(0.03192919414909);
  SET_AND_DELAY(0.032811985862145);
  SET_AND_DELAY(0.033702767020484);
  SET_AND_DELAY(0.034601466596948);
  SET_AND_DELAY(0.035508015425773);
  SET_AND_DELAY(0.036422346122674);
  SET_AND_DELAY(0.037344393009656);
  SET_AND_DELAY(0.038274092044196);
  SET_AND_DELAY(0.039211380752466);

  // Turn everything off; we're done for this cycle
  PORTA.OUT = PORTB.OUT = PORTC.OUT = PORTD.OUT = PORTE.OUT = 0xFF;

  /*
   * Now that we've done a PWM cycle, we have some time to do costly
   * calculations without worrying about interrupting the next cycle.
   * If colors have changed recently, update the port masks so they're
   * ready for the next cycle.
   */
  if (updatePortMasks) {
    ALWLeds::singleton->updatePorts();
  }
}

void ALWLeds::updatePorts() {
  byte nextLed = 0;
  byte portIndex = 0;
  nextLed = updatePort(portIndex++, 8, nextLed);
  nextLed = updatePort(portIndex++, 4, nextLed);
  nextLed = updatePort(portIndex++, 8, nextLed);
  nextLed = updatePort(portIndex++, 6, nextLed); // PD6, PD7 are used for USB
  nextLed = updatePort(portIndex++, 2, nextLed); // PE2, PE3 are reserved

  updatePortMasks = false;
}

byte ALWLeds::updatePort(uint8_t portIndex, uint8_t numPins, uint16_t firstLed) {
  for (byte curBright = 0; curBright < MAX_PWM_VALUE; curBright++) {
    byte bits = 0;
    for (byte j = 0; j < numPins; j++) {
      if ((pwmValues[firstLed + j] / (0xFF / MAX_PWM_VALUE)) >> (MAX_BRIGHTNESS - brightness) <= curBright) {
        bits |= (1 << j);
      }
    }
    ALWLeds_portMasks[(uint16_t)curBright * 5 + portIndex] = bits;
  }
  return firstLed + numPins;
}
//ISR(TCC0_CCA_vect) {
//}

ALWLeds* ALWLeds::singleton = NULL;
void ALWLeds::init(const LEDData* data, byte logicalNumber) {
  singleton = this;
  ledData = data;
  numLogicalLeds = logicalNumber;

  // All LED ports to output
  PORTA.DIR = 0xFF;
  PORTB.DIR = 0xFF;
  PORTC.DIR = 0xFF;
  PORTD.DIR = 0xFF;
  PORTE.DIR = 0xFF;

  clear();

  TCC0.CTRLA = (TCC0.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV1024_gc;
  TCC0.CTRLB = TC_WGMODE_NORMAL_gc;
  TCC0.INTCTRLA = TC_OVFINTLVL_MED_gc; // interrupt on timer overflow
//  TCC0.INTCTRLB = TC_CCAINTLVL_MED_gc; // interrupt on timer compare
  TCC0.PER = PWM_CYCLE_TIME_uS / 1000.0 / 1000.0 / PWM_ON_FRACTION * F_CPU / 1024.0;
//  TCC0.CCA = TCC0.PER / 2;
  TCC0.CNT = 0;

}

void ALWLeds::clear() {
  fill(0);
}

void ALWLeds::fill(Color color, bool useMax) {
  for (byte i = 0; i < numLogicalLeds; i++) {
    setLedColor(i, color, NULL, useMax);
  }
}

void ALWLeds::setLedColor(byte index, Color color, byte* dest, bool useMax) {
  if (!dest) {
    dest = pwmValues;
  }
  if (dest == pwmValues) {
    updatePortMasks = false;
  }
  WordToLEDData d;
  d.word = pgm_read_word(ledData + index);
  switch (d.data.type) {
    case LED_TYPE_BLUE:
    case LED_TYPE_WHITE:
      dest[d.data.portIndex] = useMax ? COLOR_USE_MAX(color) : color;
      break;
    case LED_TYPE_GBR:
      // Green byte to first pin
      dest[d.data.portIndex + 0] = color >> 8;
      // Blue byte to second pin
      dest[d.data.portIndex + 1] = color >> 0;
      // Red byte to third pin
      dest[d.data.portIndex + 2] = color >> 16;
      break;
    case LED_TYPE_RGB:
      // Red byte to first pin
      dest[d.data.portIndex + 0] = color >> 16;
      // Green byte to second pin
      dest[d.data.portIndex + 1] = color >> 8;
      // Blue byte to third pin
      dest[d.data.portIndex + 2] = color >> 0;
      break;
    case LED_TYPE_RSGB:
      // Red byte to first pin
      dest[d.data.portIndex + 0] = color >> 16;
      // Skip a pin
      // Green byte to third pin
      dest[d.data.portIndex + 2] = color >> 8;
      // Blue byte to fourth pin
      dest[d.data.portIndex + 3] = color >> 0;
      break;
    default:
      // TODO: Report an error somehow.
      break;
  }
  if (dest == pwmValues) {
    updatePortMasks = true;
  }
}

Color ALWLeds::getLedColor(byte index) {
  WordToLEDData d;
  d.word = pgm_read_word(ledData + index);

  if (d.data.type == LED_TYPE_BLUE || d.data.type == LED_TYPE_WHITE) {
    uint32_t value = pwmValues[d.data.portIndex];
    if (d.data.type == LED_TYPE_BLUE) {
      // Simple; blue of this value.
      return value;
    } else {
      // Just for fun, return the "true" color here: white of this value.
      return value << 16 | value << 8 | value;
    }
  } else {
    // Need to get 4 values because of RSGB type
    uint32_t values[4];
    for (byte i = 0; i < 4; i++) {
      values[i] = pwmValues[d.data.portIndex + i];
    }

    switch (d.data.type) {
      case LED_TYPE_GBR:
        return values[0] << 8 | values[1] << 0 | values[2] << 16;
      case LED_TYPE_RGB:
        return values[0] << 16 | values[1] << 8 | values[2] << 0;
      case LED_TYPE_RSGB:
        return values[0] << 16 | values[2] << 8 | values[3] << 0;
      default:
        // TODO: Report an error somehow.
        break;
    }
  }
  return 0;
}

FrameData ALWLeds::loadFrame(const FrameData* f, byte* dest) {
  FrameData frame;

  frame.duration = pgm_read_float(&f->duration);
  frame.transTime = pgm_read_float(&f->transTime);
  frame.transType = pgm_read_byte(&f->transType);
  frame.numColors = pgm_read_byte(&f->numColors);
  frame.colors = (const Color*)pgm_read_dword(&f->colors);

  for (byte i = 0; i < numLogicalLeds; i++) {
    setLedColor(i, pgm_read_dword(&frame.colors[i % frame.numColors]), dest);
  }
  return frame;
}

#define FADE_FRAME_STEPS 63.0
void ALWLeds::fadeFrames(const FrameData* from, const FrameData* to, delay_func_t delayFunction) {
  byte fromPwm[MAX_NUM_LEDS];
  byte toPwm[MAX_NUM_LEDS];

  FrameData fromFrame = loadFrame(from, fromPwm);
  FrameData toFrame = loadFrame(to, toPwm);

  // Start with fromPwm
  memcpy(pwmValues, fromPwm, MAX_NUM_LEDS);
  updatePortMasks = true;
  // Account for clock skew
  delayFunction(fromFrame.duration * 1.024);

  float stepSize[MAX_NUM_LEDS];
  float stepTime = fromFrame.transTime / FADE_FRAME_STEPS;
  for (byte i = 0; i < MAX_NUM_LEDS; i++) {
    stepSize[i] = (float)((int)toPwm[i] - (int)fromPwm[i]) / FADE_FRAME_STEPS;
  }
  for (float curStep = 1; curStep <= FADE_FRAME_STEPS; curStep++) {
    for (byte i = 0; i < MAX_NUM_LEDS; i++) {
      pwmValues[i] = (float)fromPwm[i] + stepSize[i] * curStep;
    }
    updatePortMasks = true;
    // Account for clock skew
    delayFunction(stepTime * 1.024);
  }

  memcpy(pwmValues, toPwm, MAX_NUM_LEDS);
  updatePortMasks = true;
}

byte ALWLeds::getBrightness() {
  return brightness;
}
byte ALWLeds::setBrightness(byte b) {
  brightness = b % (MAX_BRIGHTNESS + 1);
  updatePortMasks = true;
  return brightness;
}
byte ALWLeds::increaseBrightness() {
  if (brightness < MAX_BRIGHTNESS) {
    return setBrightness(brightness + 1);
  }
  return brightness;
}
byte ALWLeds::decreaseBrightness() {
  if (brightness > 0) {
    return setBrightness(brightness - 1);
  }
  return brightness;
}


// Fixed-point colorspace conversion: HSV (hue-saturation-value) to RGB.
// This is a bit like the 'Wheel' function from the original strandtest
// code on steroids.  The angular units for the hue parameter may seem a
// bit odd: there are 1536 increments around the full color wheel here --
// not degrees, radians, gradians or any other conventional unit I'm
// aware of.  These units make the conversion code simpler/faster, because
// the wheel can be divided into six sections of 256 values each, very
// easy to handle on an 8-bit microcontroller.  Math is math, and the
// rendering code elsehwere in this file was written to be aware of these
// units.  Saturation and value (brightness) range from 0 to 255.
Color ALWLeds::hsv2rgb(uint16_t h, byte s, byte v) {
  byte r, g, b, lo;
  int  s1;
  long v1;

  // Hue
  h %= 1536;           // -1535 to +1535
  lo = h & 255;        // Low byte  = primary/secondary color mix
  switch(h >> 8) {     // High byte = sextant of colorwheel
    case 0 : r = 255     ; g =  lo     ; b =   0     ; break; // R to Y
    case 1 : r = 255 - lo; g = 255     ; b =   0     ; break; // Y to G
    case 2 : r =   0     ; g = 255     ; b =  lo     ; break; // G to C
    case 3 : r =   0     ; g = 255 - lo; b = 255     ; break; // C to B
    case 4 : r =  lo     ; g =   0     ; b = 255     ; break; // B to M
    default: r = 255     ; g =   0     ; b = 255 - lo; break; // M to R
  }

  // Saturation: add 1 so range is 1 to 256, allowig a quick shift operation
  // on the result rather than a costly divide, while the type upgrade to int
  // avoids repeated type conversions in both directions.
  s1 = s + 1;
  r = 255 - (((255 - r) * s1) >> 8);
  g = 255 - (((255 - g) * s1) >> 8);
  b = 255 - (((255 - b) * s1) >> 8);

  // Value (brightness) and 24-bit color concat merged: similar to above, add
  // 1 to allow shifts, and upgrade to long makes other conversions implicit.
  v1 = v + 1;
  return (((r * v1) & 0xff00) << 8) |
          ((g * v1) & 0xff00)       |
         ( (b * v1)           >> 8);
}
