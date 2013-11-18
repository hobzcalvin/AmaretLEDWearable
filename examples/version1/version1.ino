#include <ALWLeds.h>
#include "ledConfig.h"
#include <ALWBase.h>


#define RANDOM_MODE_TIME_MS 15000UL

typedef void (*mode_func_t)(void);

ALWBase base;
ALWLeds leds;
enum ChangeStates {
  CHANGE_NOT = 0,
  CHANGE_BUTTON,
  CHANGE_SLEEP,
  CHANGE_RANDOM_TIMEOUT,
};
volatile ChangeStates changingMode;
volatile int8_t curMode;
unsigned long modeEndTime;

mode_func_t modes[] = {
  &userDefinedFade,
  &userDefinedCut,
//  &play,
  &spin,
//  &randDots,
  &fireflies,
  &swipes,
  &heart,
  &purpleCircle,
  &starBurst,
  &fire,
  &colorWheel,
  &slowRainbow,
};
#define NUM_MODES (sizeof(modes) / sizeof(mode_func_t))

void setup() {
  base.init();
  leds.init(ledData, NUM_LOGICAL_LEDS);

  base.buttonClickFunctions[0] = buttonOne;
  base.buttonClickFunctions[1] = buttonTwo;

  changingMode = CHANGE_NOT;
  curMode = 0;
  // XXX TEMP
//  curMode = 2;
  modeEndTime = __UINT32_MAX__;

  base.fixClockTiming();
}

void loop() {
  runModes();
}

void findTriState(byte ctrl, byte led) {
/*  PORTE.PIN3CTRL = ctrl;
  PORTE.OUTCLR = PIN3_bm;
  delay(10);
  bool lowSet = !(PORTE.IN & PIN3_bm);
  PORTE.OUTSET = PIN3_bm;
  delay(10);
  bool hiSet = PORTE.IN & PIN3_bm;

  Color c = 0;
  if (lowSet) c |= 0xff0000;
  if (hiSet) c |= 0xff;

  leds.setLedColor(led, c);*/
}

void play() {
  while (!changingMode) {
    leds.fill(0xff);
    delay(3000);
    leds.clear();
    delay(3000);
    continue;

    leds.fill(0xff0000);
    for (int i = 0; i < 5000; i++) {
      modeDelay(1);
    }
    leds.clear();
    for (int i = 0; i < 500; i++) {
      modeDelay(10);
    }
    leds.fill(0x00ff00);
    for (int i = 0; i < 50; i++) {
      modeDelay(100);
    }
    leds.clear();
    for (int i = 0; i < 5; i++) {
      modeDelay(1000);
    }
  }

  while (!changingMode) {
/*    PORTE.DIRSET = PIN3_bm;
  // START HERE: tried all sorts of crazy combinations, but forgot that the PWM code sets PORTE all the time! so all my data may be bogus. yay. try again without pwm going, or with pwm that doesn't interfere.
  PORTE.DIRCLR = PIN3_bm;
  PORTE.PIN3CTRL = 0;//PORT_OPC_TOTEM_gc;
  delay(10);
  bool naturalIn = PORTE.IN & PIN3_bm;
  leds.setLedColor(0, naturalIn ? 0x000400 : 0x040000 );*/
/*
  PORTE.DIRSET = PIN3_bm;

  //findTriState(0, 0);
  findTriState(PORT_OPC_TOTEM_gc, 1);
  findTriState(PORT_OPC_BUSKEEPER_gc, 2);
  findTriState(PORT_OPC_PULLDOWN_gc, 3);
  findTriState(PORT_OPC_PULLUP_gc, 4);
  // low/hi both work when plugged/charging; hi works when unplugged; hi works when plugged/charged
  findTriState(PORT_OPC_WIREDOR_gc, 5);
  findTriState(PORT_OPC_WIREDAND_gc, 8);
  findTriState(PORT_OPC_WIREDORPULL_gc, 7);
  // low works when plugged/charging; both work when unplugged; both work when plugged/charged
  // got charged on the other thing. put in proto, shows charged. unplug an instant and it's back to charging. stays that way. unplug/plug and maybe back to charged? sometimes?
  // okay, at super-low-brightness charging overnight, turns charged. unplug a second, uncharged, plug in, still uncharged, staying that way for now.
  findTriState(PORT_OPC_WIREDANDPULL_gc, 6);*/


  modeDelay(100);
  }
}

void buttonOne() {
  if (!leds.decreaseBrightness()) {
    changingMode = CHANGE_SLEEP;
  }
}
void buttonTwo() {
  curMode++;
  if (curMode >= (int8_t)NUM_MODES) {
    // Loop around to random
    curMode = -1;
  }
  changingMode = CHANGE_BUTTON;
}

void runModes() {
  while (true) {
    changingMode = CHANGE_NOT;
    if (curMode < 0) {
      // Random mode; signal it first.
      for (byte i = 0; i < 10; i++) {
        leds.fill(COLOR(255, 255, 255));
        modeDelay(50);
        leds.clear();
        modeDelay(50);
      }
      /*leds.fill(COLOR(255, 0, 0));
      modeDelay(500);
      leds.fill(COLOR(0, 255, 0));
      modeDelay(500);
      leds.fill(COLOR(0, 0, 255));
      modeDelay(500);
      leds.clear();*/
      modeDelay(500);

      // Random timeout ends the current random mode, but doesn't leave random
      while (!changingMode || changingMode == CHANGE_RANDOM_TIMEOUT) {
        changingMode = CHANGE_NOT;
        modeEndTime = millis() + RANDOM_MODE_TIME_MS;
        byte randMode = random(NUM_MODES);
        while (millis() < modeEndTime && !changingMode) {
          leds.clear();
          modes[randMode]();
        }
        modeEndTime = __UINT32_MAX__;
      }

    } else {
      // Normal mode operation
      leds.clear();
      modes[curMode]();
    }
    if (changingMode == CHANGE_SLEEP) {
      leds.clear();
      base.sleep();
      // Now we're awake; go to full brightness
      leds.setBrightness(MAX_BRIGHTNESS);
    }
  }
}

void modeDelay(unsigned long time) {
  time += millis();
  while (millis() < time && millis() < modeEndTime && !changingMode) {
    // This saves hella power! We'll be woken up every time the PWM cycle runs.
    base.idle();
  }
  if (millis() >= modeEndTime) {
    changingMode = CHANGE_RANDOM_TIMEOUT;
  }
}

void userDefinedFade() {
  byte numFrames = pgm_read_byte(&userPattern1.numFrames);
  for (byte i = 0; i < numFrames && !changingMode; i++) {
    leds.fadeFrames(&userPattern1.frames[i], &userPattern1.frames[(i + 1) % numFrames], modeDelay);
  }
}

void userDefinedCut() {
  byte numFrames = pgm_read_byte(&userPattern1.numFrames);
  for (byte i = 0; i < numFrames && !changingMode; i++) {
    FrameData frame = leds.loadFrame(&userPattern1.frames[i]);
    // Multiply by 1.024 because the clock runs fast by that factor.
    modeDelay((frame.duration + frame.transTime) * 1.024);
  }
}

#define SPIN_MAX_SPEED 0.1
#define SPIN_MIN_SPEED 0.0001
void spin() {
  uint16_t hue = random(MAX_HUE);
  byte sat = random(256);
  bool forward = true;
  bool accelerating = true;
  float speed = 0.01;
  float speedStep = 0.0005;
  float cur = 0;
  while (!changingMode) {
    for (byte i = 0; i < NUM_LOGICAL_LEDS; i++) {
      float dist = fabs(cur - (float)i / (float)NUM_LOGICAL_LEDS);
      if (dist > 0.5) {
        dist = 1.0 - dist;
      }
      leds.setLedColor(pgm_read_byte(radialToLedIndex + i), ALWLeds::hsv2rgb(hue, sat, pow((1.0 - dist), 30) * 255), NULL, true);
    }
    modeDelay(10);
    cur = cur + speed * (forward ? 1 : -1);
    while (cur > 1) cur--;
    while (cur < 0) cur++;
    speed = speed + speedStep * (accelerating ? 1 : -1);
    if (accelerating && speed > SPIN_MAX_SPEED) {
      accelerating = false;
      hue = random(MAX_HUE);
      sat = 128 + random(128);
    }
    if (!accelerating && speed < SPIN_MIN_SPEED) {
      accelerating = true;
      forward = !forward;
      speedStep = 0.00005 + random(101) / 100.0 * 0.0005;
    }
  }
}

void randDots() {
  while (!changingMode) {
    byte index = random(NUM_LOGICAL_LEDS);
    Color from = leds.getLedColor(index);
    Color to = ALWLeds::hsv2rgb(random(MAX_HUE), 255, 255);
    float fromR = RED(from);
    float fromG = GREEN(from);
    float fromB = BLUE(from);
    float toR = RED(to);
    float toG = GREEN(to);
    float toB = BLUE(to);

    for (float i = 0; i <= 1 && !changingMode; i += 0.03) {
      leds.setLedColor(index,
        COLOR(fromR + (toR - fromR) * i,
              fromG + (toG - fromG) * i,
              fromB + (toB - fromB) * i));
      modeDelay(1);
    }
  }
}

#define FIREFLY_BEFORE_SYNC 12000
void fireflies() {
  uint16_t curBright[NUM_LOGICAL_LEDS];
  uint16_t cycleTime[NUM_LOGICAL_LEDS];
  for (byte i = 0; i < NUM_LOGICAL_LEDS; i++) {
    cycleTime[i] = random(400, 500);
    // All curBrights will be 0 after FIREFLY_BEFORE_SYNC cycles.
    curBright[i] = cycleTime[i] - FIREFLY_BEFORE_SYNC % cycleTime[i];
  }
  while (!changingMode) {
    for (byte i = 0; i < NUM_LOGICAL_LEDS; i++) {
      leds.setLedColor(i, ALWLeds::hsv2rgb(240, 200,
        4 * (curBright[i] < 64 ? curBright[i] : (curBright[i] < 128 ? 127 - curBright[i] : 0))), NULL, true);
      curBright[i]++;
      if (curBright[i] >= cycleTime[i]) {
        curBright[i] = 0;
      }
    }
    modeDelay(10);
  }
}

void swipes() {
  const uint8_t* index = random(2) ? horizontalLedIndex : verticalLedIndex;
  bool descending = random(2);
  Color c = ALWLeds::hsv2rgb(random(MAX_HUE), random(256), 255);

  for (uint8_t i = 0; i < NUM_LOGICAL_LEDS && !changingMode; i++) {
    int cur = descending ? NUM_LOGICAL_LEDS - 1 - i : i;
    int other = cur + (descending ? 1 : -1);
    leds.clear();
    leds.setLedColor(pgm_read_byte(index + cur), c);
    if (other >= 0 && other < NUM_LOGICAL_LEDS) {
      leds.setLedColor(pgm_read_byte(index + other), c);
    }
    modeDelay(300 / NUM_LOGICAL_LEDS);
  }
  leds.clear();
  modeDelay(300);
}

void setLubDub(float cur, bool lub, float start, float peak, float end) {
  byte value;
  if (cur < start || cur > end) {
    value = 0;
  } else if (cur < peak) {
    value = (cur - start) / (peak - start) * 255.0;
  } else {
    value = (1.0 - (cur - peak) / (end - peak)) * 255.0;
  }
  for (byte i = 0; i < NUM_LOGICAL_LEDS; i++) {
    if ((i > NUM_LOGICAL_LEDS / 2 && lub) ||
        (i <= NUM_LOGICAL_LEDS / 2 && !lub)) {
      leds.setLedColor(pgm_read_byte(horizontalLedIndex + i), COLOR(value, 0, 0), NULL, true);
    }
  }
}
#define HEART_LUB_START 0
#define HEART_LUB_PEAK 0.2
#define HEART_LUB_END 0.5
#define HEART_DUB_START 0.4
#define HEART_DUB_PEAK 0.6
#define HEART_DUB_END 0.9
void heart() {
  for (float cur = 0; cur <= 1 && !changingMode; cur += 0.01) {
    setLubDub(cur, true, HEART_LUB_START, HEART_LUB_PEAK, HEART_LUB_END);
    setLubDub(cur, false, HEART_DUB_START, HEART_DUB_PEAK, HEART_DUB_END);
    modeDelay(8);
  }
}

#define WAVES_STEPS 31.0
void waves(uint16_t hue, const byte* index) {
  byte values[NUM_LOGICAL_LEDS + 1] = { 0 };
  byte startVal = 0;
  while (!changingMode) {
    values[(startVal + NUM_LOGICAL_LEDS) % (NUM_LOGICAL_LEDS + 1)] = random(256);
    for (byte curStep = 0; curStep < WAVES_STEPS && !changingMode; curStep++) {
      for (byte i = 0; i < NUM_LOGICAL_LEDS; i++) {
        int fromValue = values[(startVal + i) % (NUM_LOGICAL_LEDS + 1)];
        int toValue = values[(startVal + i + 1) % (NUM_LOGICAL_LEDS + 1)];
        leds.setLedColor(pgm_read_byte(index + i), ALWLeds::hsv2rgb(hue, 255, fromValue + (float)(toValue - fromValue) / WAVES_STEPS * (float)curStep), NULL, true);
      }
      modeDelay(1);
    }
    startVal = (startVal + 1) % (NUM_LOGICAL_LEDS + 1);
  }
}

void purpleCircle() {
  waves(1225, radialToLedIndex);
}

void starBurst() {
  byte led = random(NUM_LOGICAL_LEDS);
  for (int i = 255; i >= 0 && !changingMode; i -= 4) {
    leds.setLedColor(led, COLOR(i, i, i));
    modeDelay(2);
  }
}

void fire() {
  for (byte i = 0; i < NUM_LOGICAL_LEDS && !changingMode; i++) {
    leds.setLedColor(i, ALWLeds::hsv2rgb(random(85, 120), 255, random(128, 256)), NULL, true);
  }
  modeDelay(50);
}

void colorWheel() {
  for (uint16_t baseHue = 0; baseHue <= MAX_HUE && !changingMode; baseHue++) {
    for (uint16_t i = 0; i < NUM_LOGICAL_LEDS; i++) {
      leds.setLedColor(i, ALWLeds::hsv2rgb(baseHue + i * ((MAX_HUE + 1) / NUM_LOGICAL_LEDS)));
    }
    modeDelay(1);
  }
}

void slowRainbow() {
  for (uint16_t hue = 0; hue <= MAX_HUE && !changingMode; hue+=1) {
    leds.fill(ALWLeds::hsv2rgb((hue + 1024) % MAX_HUE));
    modeDelay(10);
  }
}

