/* Pin definitions. Use whatever pins suit your project, but
   be mindful that the 'CLK' buttons need to be connected to a
   pin with a hardware interrupt. This may vary depending
   on the exact board model. I'm using a Leonardo */
#define JS1CLKPin 2 // PD1, joystick 1 button 0, GRiP JS1 clock. Need an int 
#define JS1DATPin 4 // PD4, joystick 1 button 1, GRiP JS1 data
#define JS2CLKPin 3 // PD0, joystick 2 button 0, GRiP JS2 clock. Need an int
#define JS2DATPin 6 // PD7, joystick 2 button 1, GRiP JS2 data
#define RXLED 17 // The Leonardo RX LED
#define TXLED 30 // The Leonardo TX LED
#define GRIPSZ 24 // GRiP packetsize is 24 bytes

/*  Data packet consists of 24 bytes, as follows:
  0 1 1 1 1 1 0 Select Start R2 Blue 0 L2 Green Yellow Red 0 L1 R1 Up Down 0 Right Left
  Bitmap to express
 */
#define DECODE_left   0b00000001
#define DECODE_right  0b00000010
#define DECODE_down   0b00000100
#define DECODE_up     0b00001000
#define DECODE_r1     0b00010000
#define DECODE_l1     0b00100000
#define DECODE_red    0b01000000
#define DECODE_yellow 0b10000000

#define DECODE_green  0b00000001
#define DECODE_l2     0b00000010
#define DECODE_blue   0b00000100
#define DECODE_r2     0b00001000
#define DECODE_start  0b00010000
#define DECODE_select 0b00100000
