# GRiP-duino
Arduino project to convert 15-pin Gravis Gamepad Pro gamepads to USB-HID in digital GRiP mode (supporting all buttons)

NOTES:

Work-in-progress. Currently status:

-interrupt handlers written using registers for performance. The CLK buttons need an arduino pin with interrupt support. (see source code or notes below for pin assignments)

-supports two devices

-able to read out a bitstream from both

-able to detect the 'sync' pattern, and align the packet correctly

-Implemented change detection - only outputs *changes* to presses, instead of spitting out the whole packet every loop()

-Packets are output to the hardware USART

-Some initial performance optimization. It takes slightly longer to process a packet than it does to collect it, but it's still effectively 'polling' at 500Hz, which is plenty, probably 20x as fast as DOS games would have polled the joystick port. This will probably slow down some as I implement code to parse out the packets.

-No parsing of the packets, other than aligning them to the sync pattern. 

-Once parsing is completed, need to set up keymapping and USB output

-currently planning on mapping buttons to 'keyboard' keypresses, althogh there are libraries out there to emulate a gamepad HID device.


PINOUT:

DB-15:

pins 1,9,8,15: GND

pins 4,5,12: +5V

pin 7: Leonardo pin 4 (Joystick# 0 DATA signal)

pin 2: Leonardo pin 2 (Joystick# 0 CLK signal)

pin 10: Leonardo pin 3 (Joystick# 1 CLK signal)

pin 14: Leonardo pin 6 (Joystick# 1 DATA signal)

all other pins n/c


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
  Currently, polls about every 2ms, about 500Hz. 

  I have no idea what will happen with gamepads in non-grip mode.

  GRiP mode uses button 0 as a 20-25kHz clock signal, and button 1 as data
  Data packet consists of 24 bytes, as follows:
  0 1 1 1 1 1 0 Select Start R2 Blue 0 L2 Green Yellow Red 0 L1 R1 Up Down 0 Right Left
