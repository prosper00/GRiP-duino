/* Pin definitions. Use whatever pins suit your project, but
   be mindful that the 'CLK' buttons need to be connected to a
   pin with a hardware interrupt. This may vary depending
   on the exact board model. I'm using a Leonardo 
   
WARNING!!! If you want to change pin assignments, you will also have to edit the 
GETJSBIT handlers, as the pin assignments (port registers) are hardcoded there!
   
*/
#define JS1CLKPin 2 // PD1, joystick 1 button 0, GRiP JS1 clock. Need an int 
#define JS1DATPin 4 // PD4, joystick 1 button 1, GRiP JS1 data
#define JS2CLKPin 3 // PD0, joystick 2 button 0, GRiP JS2 clock. Need an int
#define JS2DATPin 6 // PD7, joystick 2 button 1, GRiP JS2 data
#define RXLED 17    // The Leonardo RX LED
#define TXLED 30    // The Leonardo TX LED
#define GRIPSZ 24   // GRiP packetsize is 24 bytes

/* Keymapping table: 
* Our compressed data packet consists of 2 bytes, as follows:
* 0 0 Select Start R2 Blue L2 Green Yellow Red L1 R1 Up Down Right Left
*/
//JS1 keymap
const char JS_keys[2][14] = {
  {
  (char)KEY_LEFT_ARROW,  // JS1_left   bit 0000000000000001
  (char)KEY_RIGHT_ARROW, // JS1_right  bit 0000000000000010
  (char)KEY_DOWN_ARROW,  // JS1_down   bit 0000000000000100
  (char)KEY_UP_ARROW,    // JS1_up     bit 0000000000001000
  ':',                   // JS1_r1     bit 0000000000010000
  'l',                   // JS1_l1     bit 0000000000100000
  (char)KEY_RIGHT_SHIFT, // JS1_red    bit 0000000001000000
  ' ',                   // JS1_yellow bit 0000000010000000
  (char)KEY_RIGHT_CTRL,  // JS1_green  bit 0000000100000000
  '.',                   // JS1_l2     bit 0000001000000000
  (char)KEY_RIGHT_ALT,   // JS1_blue   bit 0000010000000000
  '/',                   // JS1_r2     bit 0000100000000000
  (char)KEY_RETURN,      // JS1_start  bit 0001000000000000
  (char)KEY_ESC          // JS1_select bit 0010000000000000
  },

//JS2 keymap
  {
  'a',             // JS2_left   bit 0000000000000001
  'd',             // JS2_right  bit 0000000000000010
  's',             // JS2_down   bit 0000000000000100
  'w',             // JS2_up     bit 0000000000001000
  'e',             // JS2_r1     bit 0000000000010000
  'q',             // JS2_l1     bit 0000000000100000
  'r',             // JS2_red    bit 0000000001000000
  'f',             // JS2_yellow bit 0000000010000000
  'g',             // JS2_green  bit 0000000100000000
  '1',             // JS2_l2     bit 0000001000000000
  't',             // JS2_blue   bit 0000010000000000
  '3',             // JS2_r2     bit 0000100000000000
  'z',             // JS2_start  bit 0001000000000000
  'c'              // JS2_select bit 0010000000000000
  }
};


