# GRiP-duino
Arduino project to convert 15-pin Gravis Gamepad Pro gamepads to USB-HID in digital GRiP mode (supporting all buttons)

  GRiP to HID joystick converter for the Gravis Gamepad Pro

  Needs an Arduino or compatible device with native USB.
  Devices built with an ATmega32u4 should work, but may need modification.
  This code is built and tested on a Leonardo.

  The gamepad pro has a switch on the back, and has 3 modes.
  in mode 1, only two buttons per gamepad are used
  in mode 2, all four buttons are used by gamepad 1, and gamepad 2 is disabled
  in mode 3 (GRiP), up to four gamepads can be used at once, and all 10 buttons are available each
  This code only supports two devices, mostly because I only HAVE two to test with. Also,
  we're already taxing the atmega with two devices (and my inefficient code)
  Currently, polls about every 2ms, about 500Hz

  I have no idea what will happen with gamepads in non-grip mode.

  GRiP mode uses button 0 as a 20-25kHz clock signal, and button 1 as data
  Data packet consists of 24 bytes, as follows:
  0 1 1 1 1 1 0 Select Start R2 Blue 0 L2 Green Yellow Red 0 L1 R1 Up Down 0 Right Left
