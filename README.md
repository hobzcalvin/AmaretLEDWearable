AmaretLEDWearable
=================

Installation
------------

Put all of these files in:
    { path to arduino projects }/libraries/AmaretLEDWearable/

Version 1 of the LED wearable firmware will appear as an example in Arduino under AmaretLEDWearable.
The ALWBase library handles button detection with debouncing, power management, and other low-level microprocessor stuff so you don't have to.
The ALWLeds library lets you set LED color and overall brightness; it does the rest to properly dim the LEDs (using a custom non-linear PWM algorithm).
You will need an ledConfig.h file corresponding to your wearable; this tells the code how many LEDs you have, what color(s) they are, and to what pins they are connected.
