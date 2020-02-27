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

   //initialize serial at 2 meg. I assume that faster speeds means we spend less time on I/O.
   //works fine with my CH341 adapter, though occasionally get some corruption or missed chars.
  Serial1.begin(2000000);  //This is the UART, on hardware pins 0 and 1.
  Serial1.println(F("Initialize Serial Hardware UART Pins"));

  attachInterrupt(digitalPinToInterrupt(JS1CLKPin), GETJS1BIT, FALLING);
  attachInterrupt(digitalPinToInterrupt(JS2CLKPin), GETJS2BIT, FALLING);
  delay(500); //wait for things to stabilize and the gamepads themselves to bootup and fill the buffers

  Keyboard.begin();
}

void loop() {
    static uint32_t JS1packet, JS2packet; // static to preserve the variable for the next loop
    uint32_t JS1previous = JS1packet;
    uint32_t JS2previous = JS2packet;
    uint32_t JS1Sync, JS2Sync;

    noInterrupts();  //copying the buffers takes multiple cycles and we can't have the interrupts writing data
    JS1Sync = JS1buff;
    JS2Sync = JS2buff;
    interrupts();
    
    //TODO: would this be significantly faster if I passed by reference instead of value?
    JS1packet = SyncPacket(JS1Sync, JS1previous);
    JS2packet = SyncPacket(JS2Sync, JS2previous);

    if (JS1packet != JS1previous) { //if no joypad keypresses have *changed*, we don't need to send anything
      SendKeys(JS1packet, JS1previous, 1);
    }
    if (JS2packet != JS2previous) {
      SendKeys(JS2packet, JS2previous, 2);
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
  while ((buff & 0x00FE1084) != 0x007C0000) { //FE1084 masks the bits we care about, 7C is our 'sync' pattern
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
*/
void SendKeys(uint32_t packet, uint32_t previous, byte JSnum) { //JSnum = 1 for the first JS, and 2 for the 2nd
  /* this bit makes my brain hurt, but, what I'm doing here is compressing our 24-bit packet stuffed into
   *  a 32-bit ulong into 2 8-bit bytes by eliminating the '0's used for framing, and the 0111110 sync bit. 
   *  Really we only CARE about 14 bits and the AVR can only work with 8-bits at a time, so each 'if' 
   *  statement comparing two 32-bit longs takes a lot more ops than comparing 8-bit bytes. 
   *  I should move this into SyncPacket...
   */
  byte cmp_packet[2]={0,0}; 
  cmp_packet[0] = ( (packet & 0b11) | ( (packet>>1) & 0b111100) | ( (packet>>2) & 0b11000000) );
  cmp_packet[1] = ( ( (packet>>10) & 0b11) | ( (packet >>11) & 0b111100) );

  byte cmp_previous[2]={0,0};
  cmp_previous[0] = ( (previous & 0b11) | ( (previous>>1) & 0b111100) | ( (previous>>2) & 0b11000000) );
  cmp_previous[1] = ( ( (previous>>10) & 0b11) | ( (previous >>11) & 0b111100) );

  byte mask[2];
  mask[0]=cmp_packet[0]^cmp_previous[0];    //any bits that have *changed* since last time are masked
  mask[1]=cmp_packet[1]^cmp_previous[1];

  if (JSnum == 1) {
      digitalWrite(TXLED, LOW);  // turn on the LED
      if (mask[0] & DECODE_left){     //if the 'left' d-pad state has changed
        if(mask[0] & cmp_packet[0]){  //if the bit in our packet is '1' 
          Keyboard.press(JS1_left);
          Serial1.println(F(" JS1: Left Pressed "));}
        else{                         //otherwise...
         Keyboard.release(JS1_left);
         Serial1.println(F(" JS1: Left Released "));}}
      if (mask[0] & DECODE_right){
        if(mask[0] & cmp_packet[0]){
          Keyboard.press(JS1_right);
          Serial1.println(F(" JS1: Right Pressed "));}
        else{
          Keyboard.release(JS1_right);
          Serial1.println(F(" JS1: Right Released "));}}
      if (mask[0] & DECODE_down){  
        if(mask[0] & cmp_packet[0]){
          Keyboard.press(JS1_down);
          Serial1.println(F(" JS1: Down Pressed "));}
        else{
          Keyboard.release(JS1_down);
          Serial1.println(F(" JS1: Down Released "));}}
      if (mask[0] & DECODE_up){
        if(mask[0] & cmp_packet[0]){
          Keyboard.press(JS1_up);
          Serial1.println(F(" JS1: Up Pressed "));}
        else{
          Keyboard.release(JS1_up);
          Serial1.println(F(" JS1: Up Released "));}}
      if (mask[0] & DECODE_r1){
        if(mask[0] & cmp_packet[0]){
          Keyboard.press(JS1_r1);
          Serial1.println(F(" JS1: R1 Pressed "));}
        else{
          Keyboard.release(JS1_r1);
          Serial1.println(F(" JS1: R1 Released "));}}
      if (mask[0] & DECODE_l1){
        if(mask[0] & cmp_packet[0]){
          Keyboard.press(JS1_l1);
          Serial1.println(F(" JS1: L1 Pressed "));}
        else{
          Keyboard.release(JS1_l1);
          Serial1.println(F(" JS1: L1 Released "));}}
      if (mask[0] & DECODE_red){
        if(mask[0] & cmp_packet[0]){
          Keyboard.press(JS1_red);
          Serial1.println(F(" JS1: Red Pressed "));}
        else{
          Keyboard.release(JS1_red);
          Serial1.println(F(" JS1: Red Released "));}}
      if (mask[0] & DECODE_yellow){
        if(mask[0] & cmp_packet[0]){
          Keyboard.press(JS1_yellow);
          Serial1.println(F(" JS1: Yellow Pressed "));}
        else{
          Keyboard.release(JS1_yellow);
          Serial1.println(F(" JS1: Yellow Released "));}}
      if (mask[1] & DECODE_green){
        if(mask[1] & cmp_packet[1]){
          Keyboard.press(JS1_green);
          Serial1.println(F(" JS1: Green Pressed "));}
        else{
          Keyboard.release(JS1_green);
          Serial1.println(F(" JS1: Green Released "));}}
      if (mask[1] & DECODE_l2){
        if(mask[1] & cmp_packet[1]){
          Keyboard.press(JS1_l2);
          Serial1.println(F(" JS1: L2 Pressed "));}
        else{
          Keyboard.release(JS1_l2);
          Serial1.println(F(" JS1: L2 Released "));}}
      if (mask[1] & DECODE_blue){
        if(mask[1] & cmp_packet[1]){
          Keyboard.press(JS1_blue);
          Serial1.println(F(" JS1: Blue Pressed "));}
        else{
          Keyboard.release(JS1_blue);
          Serial1.println(F(" JS1: Blue Released "));}}
      if (mask[1] & DECODE_r2){
        if(mask[1] & cmp_packet[1]){
          Keyboard.press(JS1_r2);
          Serial1.println(F(" JS1: R2 Pressed "));}
        else{
          Keyboard.release(JS1_r2);
          Serial1.println(F(" JS1: R2 Released "));}}
      if (mask[1] & DECODE_start){
        if(mask[1] & cmp_packet[1]){
          Keyboard.press(JS1_start);
          Serial1.println(F(" JS1: Start Pressed "));}
        else{
          Keyboard.release(JS1_start);
          Serial1.println(F(" JS1: Start Released "));}}
      if (mask[1] & DECODE_select){
        if(mask[1] & cmp_packet[1]){
         Keyboard.press(JS1_select);
         Serial1.println(F(" JS1: Select Pressed "));}
        else{
          Keyboard.release(JS1_select);
          Serial1.println(F(" JS1: Select Released "));}}
    if((cmp_packet[0]|cmp_packet[1])==0){  // if no buttons are pressed
      digitalWrite(TXLED, HIGH);           // turn of the LED
    }
  }

  else   if (JSnum == 2) {
      digitalWrite(RXLED, LOW);  // turn on the LED
      if (mask[0] & DECODE_left){     //if the 'left' d-pad state has changed
        if(mask[0] & cmp_packet[0]){  //if the bit in our packet is '1' 
          Keyboard.press(JS2_left);
          Serial1.println(F(" JS2: Left Pressed "));}
        else{                         //otherwise...
          Keyboard.release(JS2_left);
          Serial1.println(F(" JS2: Left Released "));}}
      if (mask[0] & DECODE_right){
        if(mask[0] & cmp_packet[0]){
          Keyboard.press(JS2_right);
          Serial1.println(F(" JS2: Right Pressed "));}
        else{
          Keyboard.release(JS2_right);
          Serial1.println(F(" JS2: Right Released "));}}
      if (mask[0] & DECODE_down){  
        if(mask[0] & cmp_packet[0]){
          Keyboard.press(JS2_down);
          Serial1.println(F(" JS2: Down Pressed "));}
        else{
          Keyboard.release(JS2_down);
          Serial1.println(F(" JS2: Down Released "));}}
      if (mask[0] & DECODE_up){
        if(mask[0] & cmp_packet[0]){
          Keyboard.press(JS2_up);
          Serial1.println(F(" JS2: Up Pressed "));}
        else{
          Keyboard.release(JS2_up);
          Serial1.println(F(" JS2: Up Released "));}}
      if (mask[0] & DECODE_r1){
        if(mask[0] & cmp_packet[0]){
          Keyboard.press(JS2_r1);
          Serial1.println(F(" JS2: R1 Pressed "));}
        else{
          Keyboard.release(JS2_r1);
          Serial1.println(F(" JS2: R1 Released "));}}
      if (mask[0] & DECODE_l1){
        if(mask[0] & cmp_packet[0]){
          Keyboard.press(JS2_l1);
          Serial1.println(F(" JS2: L1 Pressed "));}
        else{
          Keyboard.release(JS2_l1);
          Serial1.println(F(" JS2: L1 Released "));}}
      if (mask[0] & DECODE_red){
        if(mask[0] & cmp_packet[0]){
          Keyboard.press(JS2_red);
          Serial1.println(F(" JS2: Red Pressed "));}
        else{
          Keyboard.release(JS2_red);
          Serial1.println(F(" JS2: Red Released "));}}
      if (mask[0] & DECODE_yellow){
        if(mask[0] & cmp_packet[0]){
          Keyboard.press(JS2_yellow);
          Serial1.println(F(" JS2: Yellow Pressed "));}
        else{
          Keyboard.release(JS2_yellow);
          Serial1.println(F(" JS2: Yellow Released "));}}
      if (mask[1] & DECODE_green){
        if(mask[1] & cmp_packet[1]){
          Keyboard.press(JS2_green);
          Serial1.println(F(" JS2: Green Pressed "));}
        else{
          Keyboard.release(JS2_green);
          Serial1.println(F(" JS2: Green Released "));}}
      if (mask[1] & DECODE_l2){
        if(mask[1] & cmp_packet[1]){
          Keyboard.press(JS2_l2);
          Serial1.println(F(" JS2: L2 Pressed "));}
        else{
          Keyboard.release(JS2_l2);
          Serial1.println(F(" JS2: L2 Released "));}}
      if (mask[1] & DECODE_blue){
        if(mask[1] & cmp_packet[1]){
          Keyboard.press(JS2_blue);
          Serial1.println(F(" JS2: Blue Pressed "));}
        else{
          Keyboard.release(JS2_blue);
          Serial1.println(F(" JS2: Blue Released "));}}
      if (mask[1] & DECODE_r2){
        if(mask[1] & cmp_packet[1]){
          Keyboard.press(JS2_r2);
          Serial1.println(F(" JS2: R2 Pressed "));}
        else{
          Keyboard.release(JS2_r2);
          Serial1.println(F(" JS2: R2 Released "));}}
      if (mask[1] & DECODE_start){
        if(mask[1] & cmp_packet[1]){
          Keyboard.press(JS2_start);
          Serial1.println(F(" JS2: Start Pressed "));}
        else{
          Keyboard.release(JS2_start);
          Serial1.println(F(" JS2: Start Released "));}}
      if (mask[1] & DECODE_select){
        if(mask[1] & cmp_packet[1]){
          Keyboard.press(JS2_select);
          Serial1.println(F(" JS2: Select Pressed "));}
        else{
          Keyboard.release(JS2_select);
          Serial1.println(F(" JS2: Select Released "));}}
    if((cmp_packet[0]|cmp_packet[1])==0){  // if both halves of our packet are empty, no buttons are pressed
      digitalWrite(RXLED, HIGH);           // turn off the LED
    }
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
