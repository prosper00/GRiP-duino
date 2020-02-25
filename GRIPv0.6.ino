/*
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
*/


//Includes. Need Keyboard for JS->KB emulation, and/or a lib for JS HID
#include <Keyboard.h>

/* Pin definitions. Use whatever pins suit your project, but
   be mindful that the '0' buttons need to be connected to a
   pin with a hardware interrupt. This may vary depending
   on the exact board model. I'm using a Leonardo */
#define JS1CLKPin 2 // PD1, joystick 1 button 0, GRiP JS1 clock. Need an int 
#define JS1DATPin 4 // PD4, joystick 1 button 1, GRiP JS1 data
#define JS2CLKPin 3 // PD0, joystick 2 button 0, GRiP JS2 clock. Need an int
#define JS2DATPin 6 // PD7, joystick 2 button 1, GRiP JS2 data
#define RXLED 17 // The Leonardo RX LED
#define TXLED 30 // The Leonardo TX LED
#define GRIPSZ 24 // GRiP packetsize is 24 bytes
#define LOOP_INTERVAL 2000 //how many usec to loop (min). Too low, and we get bounce and bitstream issues
                           //need to ensure we have enough time to fill up the buffers (about 1500usec)

/* Global definitions for the interrupt handler */
volatile uint32_t JS1buff = 0; //the buffer where the handler shifts the bits
volatile uint32_t JS2buff = 0;

unsigned long time_0 = 0; //for tracking performance, and our loop() delay

void setup() {
  //  DDRD&=B01101100; //sets PORTD pins 0,1,4 and 7 (Board # D3, D2, D4, D6) to input
  //  PORTD|=B10010011; //sets pullup on PORTD pins 0, 1, 4, 7 (Board # D3, D2, D4, D6)

  //enable all the inputs I'll need, and activate the internal pullup resistor.
  pinMode(JS1CLKPin, INPUT_PULLUP);
  pinMode(JS1DATPin, INPUT_PULLUP);
  pinMode(JS2CLKPin, INPUT_PULLUP);
  pinMode(JS2DATPin, INPUT_PULLUP);

  pinMode(TXLED, OUTPUT);
  pinMode(RXLED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial1.begin(2000000);  //This is the UART, on hardware pins 0 and 1.
  Serial1.println(F("Initialize Serial Hardware UART Pins"));

  attachInterrupt(digitalPinToInterrupt(JS1CLKPin), GETJS1BIT, FALLING);
  attachInterrupt(digitalPinToInterrupt(JS2CLKPin), GETJS2BIT, FALLING);
  delay(5); //initial delay to fill up the buffers
}

void loop() {
  if (micros() - time_0 >= LOOP_INTERVAL) { //Make sure we're not running any more often than LOOP_INTERVAL usec.
    time_0 = micros();
    
    static uint32_t JS1packet, JS2packet; // static to preserve the variable for the next loop
    uint32_t JS1previous = JS1packet;
    uint32_t JS2previous = JS2packet;
    uint32_t JS1Sync, JS2Sync;

    JS1Sync = JS1buff; //make a copy of the buffers, so that the interrupt doesn't overwrite it while we're parsing
    JS2Sync = JS2buff;
    JS1packet = SyncPacket(JS1Sync, JS1previous);
    JS2packet = SyncPacket(JS2Sync, JS2previous);

    if (JS1packet != JS1previous) { //if no joypad keypresses have *changed*, we don't need to send anything
      SendKeys(JS1packet, 1);
    }
    if (JS2packet != JS2previous) {
      SendKeys(JS2packet, 2);
    }

    JS1buff = 0; // clear the buffers
    JS2buff = 0;
  }
}

/*
   This sub rotates though our captured data to find the synchronization bits.
   once the bits are in the right position, exits. If we can't find sync,
   then we just return the previous packet. Rotating bits like this means we'll be getting some button
   presses from the 'last' packet mixed into the 'current' packet, but this only means about 1ms of 
   error.
*/
uint32_t SyncPacket(uint32_t buff,  uint32_t previous) {
  byte i = 1;
  while ((buff & 0x00FE1084) != 0x007C0000) { //FE1084 masks the bits we care about, 0x7C is our 'sync' pattern
    if (i > GRIPSZ) { // if we've rotated through the entire buffer and not found sync
      return previous;
    }
    buff = rotl24(buff); //rotate the entire buffer by 1 bit, and re-test
    i++;
  }
  return buff & 0x00FFFFFF; //we only care about the bottom 24 bits.
}

/*Ok, so, at this point we have a nicely aligned GRiP packet.
 * SendKeys() will parse and send it out as keypresses.
       Packet format, left-to-right, is
         0 1 1 1 1 1 0 Select Start R2 Blue 0 L2 Green Yellow Red 0 L1 R1 Up Down 0 Right Left
       Mask for the bits we WANT is 000000011110111101111011, or 0x1EF7B
       We could do some bitmap parsing to figure out which button(s) are pressed
       i.e. 0x1EF7B means that ALL buttons are pressed, subtract a 1 from each appropriate position
       to detect which buttons AREN'T pressed. However, it's unlikely that we can send 10 simultaneous
       keypresses vie USB HID (TODO: what IS the limit?)   */
void SendKeys(uint32_t packet, byte JSnum) { //JSnum = 1 for the first JS, and 2 for the 2nd
  if (JSnum == 1) {
    Serial1.print("JS1 event: ");
    Serial1.print(packet, BIN);
    Serial1.print(" usec:");
    Serial1.println(micros() - time_0);
  }
  else if (JSnum == 2) {
    Serial1.print("JS2 event: ");
    Serial1.print(packet, BIN);
    Serial1.print(" usec:");
    Serial1.println(micros() - time_0);
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
  //JS1buff|=digitalRead(JS1DATPin); //slower, but works. May need to tune the LOOP_INTERVAL
}

void GETJS2BIT () {
  JS2buff <<= 1;
  JS2buff |= ((PIND >> 7) & 1); //PIND7 is the register for JS2 data
  //JS2buff|=!digitalRead(JS2DATPin); //slower, but works.
}
