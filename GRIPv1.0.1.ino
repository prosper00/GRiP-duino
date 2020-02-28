/*
  GRiP to HID joystick converter for the Gravis Gamepad Pro

  This code is built and tested on a Leonardo. 
  Devices built with an ATmega32u4 should work, but may need modification.
  Faster devices may not work, as this depends on packet processing taking 
  enough time for the interrupts to collect a whole new packet in the background

  The gamepad pro has a switch on the back, and has 3 modes.
  in mode 1, only two buttons per gamepad are used
  in mode 2, all four buttons are used by gamepad 1, and gamepad 2 is disabled
  in mode 3 (GRiP), up to four gamepads can be used at once, and all 10 buttons are available each
  This code only supports two devices, mostly because I only HAVE two to test with. Also,
  we're already taxing the atmega with two devices (and my inefficient code)
  Currently, polls about every 2ms, about 500Hz, with 2 gamepads.

  I have no idea what will happen with gamepads in non-grip mode. Probably nothing.

  GRiP mode uses button 0 as a 20-25kHz clock signal, and button 1 as data
  Data packet consists of 24 bytes, as follows:
  0 1 1 1 1 1 0 Select Start R2 Blue 0 L2 Green Yellow Red 0 L1 R1 Up Down 0 Right Left
*/

//Includes. Need Keyboard for JS->KB emulation, and/or a lib for JS HID
#include <Keyboard.h>
#include "grip.h" // pin definitions, grip packet definition

/* Global definitions for the interrupt handler */
volatile uint32_t JS1buff = 0; //the buffer where the handler shifts the bits
volatile uint32_t JS2buff = 0;

void setup() {
  //enable all the inputs I'll need, and activate the internal pullup resistor.
  pinMode(JS1CLKPin, INPUT_PULLUP);
  pinMode(JS1DATPin, INPUT_PULLUP);
  pinMode(JS2CLKPin, INPUT_PULLUP);
  pinMode(JS2DATPin, INPUT_PULLUP);

  pinMode(TXLED, OUTPUT);
  pinMode(RXLED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

   //initialize USB-serial
  Serial.begin(230400);  //This is the USB virtual UART.
  Serial.println(F("Initialize Serial Hardware UART Pins"));

  attachInterrupt(digitalPinToInterrupt(JS1CLKPin), GETJS1BIT, FALLING);
  attachInterrupt(digitalPinToInterrupt(JS2CLKPin), GETJS2BIT, FALLING);
  delay(500); //wait for things to stabilize and the gamepads themselves to bootup and fill the buffers

  Keyboard.begin();
}

void loop() {
    static uint16_t JS1packet, JS2packet; // static to preserve the variable for the next loop
    uint16_t JS1previous = JS1packet;
    uint16_t JS2previous = JS2packet;
    uint32_t JS1Sync, JS2Sync;

    //process JS1 data
    noInterrupts();  //copying the buffer takes multiple cycles and we can't have the interrupts writing data
    JS1Sync = JS1buff;
    interrupts();
    JS1packet = SyncPacket(JS1Sync, JS1previous);
    if (JS1packet != JS1previous) { //if no joypad keypresses have *changed*, we don't need to send anything
      SendKeys(JS1packet, JS1previous, 0);
    }

    //process JS2 data
    noInterrupts();
    JS2Sync = JS2buff;
    interrupts();
    JS2packet = SyncPacket(JS2Sync, JS2previous);
    if (JS2packet != JS2previous) {
      SendKeys(JS2packet, JS2previous, 1);
    }

}

/*
   This sub rotates though our captured data to find the synchronization bits.
   once the bits are in the right position, exits. If we can't find sync,
   then we just return the previous packet. Rotating bits like this means we'll be getting some button
   presses from the 'last' packet mixed into the 'current' packet, but this only means about 1ms of 
   error.
*/
uint16_t SyncPacket(uint32_t buff,  uint16_t previous) {
  byte i = 1;
  while ((buff & 0x00FE1084) != 0x007C0000) { //FE1084 masks the bits we care about, 7C is our 'sync' pattern
    if (i > GRIPSZ) { // if we've rotated through the entire buffer and not found sync
      return previous;
    }
    buff = rotl24(buff); //rotate the entire buffer by 1 bit, and re-test
    i++;
  }
  //Compress the packet down to fit into an int, by
  //stripping out the sync bits and the framing zeros.
  //hopefully, this is more efficient than passing around 32 bits
  return ( (buff     & 0b0000000000000011) | 
         ( (buff>>1) & 0b0000000000111100) | 
         ( (buff>>2) & 0b0000001111000000) | 
         ( (buff>>3) & 0b0011110000000000) );
}

/*Ok, so, at this point we have a nicely aligned GRiP packet.
 * SendKeys() will parse and send it out as keypresses.
*/
void SendKeys(uint32_t packet, uint32_t previous, byte JSnum) { //JSnum = 1 for the first JS, and 2 for the 2nd

  uint16_t mask=packet^previous;

  for (int i = 0; i < 14; i++) {
    if (mask & 0x01) {
      if (packet & 0x01) {
        Keyboard.press(JS_keys[JSnum][i]);
      }
      else {
        Keyboard.release(JS_keys[JSnum][i]);
      }
    }
    mask >>= 1;
    packet  >>= 1;
  }
}

//rotates LSB24 bits left 1 position, from a 32-bit unsigned long
inline uint32_t rotl24(uint32_t n) {
  return (n << 1) | (n >> (24-1));
}

/* Interrupt handlers. 
 *  We're shifting the buffer each time, so that we don't have to worry about preventing overruns
 *  If parsing is taking too long, we just keep on capturing bits and shifting the oldest bits off 
 *  the end of the buffer and into oblivion.
*/
void GETJS1BIT () {
  JS1buff <<= 1;
  JS1buff |= ((PIND >> 4) & 1); //PIND4 is the register for JS1 data
  //JS1buff|=digitalRead(JS1DATPin); //slower, but works.
}

void GETJS2BIT () {
  JS2buff <<= 1;
  JS2buff |= ((PIND >> 7) & 1); //PIND7 is the register for JS2 data
  //JS2buff|=!digitalRead(JS2DATPin); //slower, but works.
}