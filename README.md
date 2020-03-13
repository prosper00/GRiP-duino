# GRiP-duino
Arduino project to convert 15-pin Gravis Gamepad Pro gamepads to USB-HID in digital GRiP mode (supporting all buttons)

NOTES:

Edit the keyboard:button mapping in the grip.h file to suit your needs.


PINOUT:

DB-15:

pins 1,9,8,15: +5V

pins 4,5,12: GND

pin 7: Leonardo pin 4 (Joystick# 0 DATA signal)

pin 2: Leonardo pin 2 (Joystick# 0 CLK signal)

pin 10: Leonardo pin 3 (Joystick# 1 CLK signal)

pin 14: Leonardo pin 6 (Joystick# 1 DATA signal)

all other pins n/c


  Needs an Arduino or compatible device with native USB.
  Devices built with an ATmega32u4 should work, but may need modification.
  This code is built and tested on a Leonardo, and a clone 'pro micro' with a 32u4.

  The gamepad pro has a switch on the back, and has 3 modes.
  in mode 1, only two buttons per gamepad are used
  in mode 2, all four buttons are used by gamepad 1, and gamepad 2 is disabled
  in mode 3 (GRiP), up to four gamepads can be used at once, and all 10 buttons are available each
  This code only supports two devices, mostly because I only HAVE two to test with. Also,
  we're already taxing the atmega with two devices.
  
  I measured this taking about 220usec per cycle through loop(), which means we're reporting at 4500Hz with two gamepads connected, for a refresh rate of 2250Hz for each gamepad. This seems sufficient :) so I'm not planning on implementing any further optimizations. This is about 3500ops per poll, which seems reasonable.
  
  I have no idea what will happen with gamepads in non-grip mode.

  GRiP mode uses button 0 as a 20-25kHz clock signal, and button 1 as data
  Data packet consists of 24 bytes, as follows:
  0 1 1 1 1 1 0 Select Start R2 Blue 0 L2 Green Yellow Red 0 L1 R1 Up Down 0 Right Left
