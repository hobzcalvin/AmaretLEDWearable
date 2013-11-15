/*
  ALWBase.cpp - Library for flashing ALWBase code.
  Created by David A. Mellis, November 2, 2007.
  Released into the public domain.
*/

#include "ALWBase.h"
#include "Arduino.h"
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <util/delay.h>

#define BUTTON_PORT PORTR
#define B0_PIN 0
#define B1_PIN 1

#define DEBOUNCE_TIME_MS 200UL

ISR(PORTR_INT0_vect) {
  ALWBase::singleton->buttonInterrupt(0);
}
ISR(PORTR_INT1_vect) {
  ALWBase::singleton->buttonInterrupt(1);
}

ALWBase::ALWBase() {
  /*
   * XXX: For some reason, this messes things up if it runs in init().
   * It must have something to do with the timing: Calling it before
   * Arduino's core init code is okay, but after is not. Bizarre.
   * One thread suggested it's the WDT, but I don't think so.
   */
  static uint32_t EEMEM storedRandomSeed;
  uint32_t randomSeed = eeprom_read_dword(&storedRandomSeed);
  srandom(randomSeed);
  eeprom_write_dword(&storedRandomSeed, random());
}

ALWBase* ALWBase::singleton = NULL;
void ALWBase::init() {
  singleton = this;
  lastButton = 0xFF;
  debounceTarget = 0;
  inSleep = false;

  // Pretty sure this is off already, but let's be positive.
  while (WDT_STATUS & 1) {};
  CCP = CCP_IOREG_gc; // Magic number to allow changing WDT_CTRL
  WDT_CTRL = WDT_CEN_bm;

  for (byte i = 0; i < NUM_BUTTONS; i++) {
    buttonClickFunctions[i] = ENTER_BOOTLOADER_FUNCTION;
  }
  allButtonsFunction = ENTER_BOOTLOADER_FUNCTION;

  byte mask;

  // Turn off unnecessary peripherals.
  PR_PRGEN = PR_AES_bm | PR_EBI_bm | PR_DMA_bm;
  // For each port, too.
  mask = PR_DAC_bm | PR_ADC_bm | PR_AC_bm;
  PR_PRPA = mask;
  PR_PRPB = mask;
  // Keep TCC0 on for PWM
  mask = PR_TWI_bm | PR_USART1_bm | PR_USART0_bm | PR_SPI_bm | PR_HIRES_bm | PR_TC1_bm;
  PR_PRPC = mask;
  // TCx0 off for other ports
  mask |= PR_TC0_bm;
  PR_PRPD = mask;
  PR_PRPE = mask;
  PR_PRPF = mask;

  // TODO: Make sure the BOD is off too, using fuses.

 // Setup falling-edge interrupt on buttons
  BUTTON_PORT.DIR = 0;
  mask = PORT_OPC_PULLUP_gc | PORT_ISC_FALLING_gc;
  BUTTON_PORT.PIN0CTRL = mask;
  BUTTON_PORT.PIN1CTRL = mask;

  BUTTON_PORT.INT0MASK = (1 << B0_PIN);
  BUTTON_PORT.INT1MASK = (1 << B1_PIN);

  BUTTON_PORT.INTCTRL = PORT_INT0LVL_MED_gc | PORT_INT1LVL_MED_gc;
  PMIC.CTRL |= PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;

  CCP = CCP_IOREG_gc; // Magic number to allow setting of the IVSEL bit
  // Take interrupt control (the bootloader probably took it from us)
  PMIC.CTRL &= ~PMIC_IVSEL_bm;

  // Turn on interrupts
  sei();

  // If we came from a BOD reset, sleep.
  if (RST_STATUS & RST_BORF_bm && !(RST_STATUS & RST_PORF_bm)) {
    // XXX: Needed? Go elsewhere??
    PORTA.OUT = 0xFF;
    PORTB.OUT = 0xFF;
    PORTC.OUT = 0xFF;
    PORTD.OUT = 0xFF;
    PORTE.OUT = 0xFF;
    PORTA.DIR = 0xFF;
    PORTB.DIR = 0xFF;
    PORTC.DIR = 0xFF;
    PORTD.DIR = 0xFF;
    PORTE.DIR = 0xFF;
    sleep();
  }
  // Clear all flags
  RST_STATUS = 0xFF;
}

/*
 * XXX: I think the xmegaduino code in wiring.c is wrong: it sets PER to 0.
 * This results in "milliseconds" taking only about .7ms, making everything
 * too fast. Setting PER to 1, which makes sense to me, fixes this.
 * Confusingly, setting RTC_PRESCALER_DIV1_gc in RTC.CTRL also fixes it.
 * More confusingly, ISR(RTC_OVF_vect) adds 4 to rtc_millis every time, which
 * seems like it ought to be counting 4 times too fast.
 * This needs a full looking at, but for now, this (which needs to happen
 * after Arduino's base init() code runs) makes the timing mostly okay.
 * It is still a tad slow: I see 9.8s for what should be 10s.
 * Edit: I know what's up.
 */
void ALWBase::fixClockTiming() {
  do {
    /* Wait until RTC is not busy. */
  } while (RTC.STATUS & RTC_SYNCBUSY_bm);
  /* Configure RTC period to 1 millisecond. */
  RTC.PER = 3; // This is a fix from the xmegaduino core, which sets it to 0.
  RTC.CNT = 0; // Necessary for the change to propagate
}

void ALWBase::buttonInterrupt(byte button) {
  // Compare button states because we may be getting the second of both buttons down.
  if (lastButton == button &&
      // Debounce.
      millis() < debounceTarget) {
    return;
  }
  // Abort if this interrupt was triggered while we're trying to sleep.
  if (inSleep) {
    return;
  }
  lastButton = button;
  debounceTarget = millis() + DEBOUNCE_TIME_MS;

  if (!(BUTTON_PORT.IN & ((1<<B0_PIN) | (1<<B1_PIN)))) {
    allButtonsFunction();
  } else {
    buttonClickFunctions[button]();
  }
}

void ALWBase::sleep() {
  // Turn off interrupts while we do cleanup stuff
  cli();

  // Wait for buttons to be high again
  while (!(BUTTON_PORT.IN & (1 << B0_PIN)) ||
         !(BUTTON_PORT.IN & (1 << B1_PIN)));
  // Wait for debounce on the lifting of the button
  _delay_ms(DEBOUNCE_TIME_MS);

  // Set buttons to trigger on low level (needed to work in power-down mode)
  byte mask = PORT_OPC_PULLUP_gc | PORT_ISC_LEVEL_gc;
  BUTTON_PORT.PIN0CTRL = mask;
  BUTTON_PORT.PIN1CTRL = mask;

  // Prevent interrupts from firing until we're really woken up
  inSleep = true;

  // Everything's set; interrupts back on
  sei();

  // Sleep
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode();

  // Wakeup may or may not trigger an interrupt; make sure it doesn't go through.
  // First, update debounceTarget.
  debounceTarget = millis() + DEBOUNCE_TIME_MS;
  // Restore falling-edge interrupt
  mask = PORT_OPC_PULLUP_gc | PORT_ISC_FALLING_gc;
  BUTTON_PORT.PIN0CTRL = mask;
  BUTTON_PORT.PIN1CTRL = mask;
  // Second, we set inSleep earlier. Now we know it's safe to unset it,
  // since debounceTarget will prevent the interrupt if necessary.
  inSleep = false;
}

void ALWBase::idle() {
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_mode();
}
