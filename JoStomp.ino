
/**************************************************************************

  BSD 3-Clause License

  Copyright (c) 2020, Matthias Wientapper
  All rights reserved.
  Customized by Jonas FORLOT
 

  Midi Foot Switch for HX Stomp
  =============================
  - Button 1/2/3 on pins D6, D7,D8
  - LED RGB green red blue       D3, D4, D5
  - requires OneButton lib https://github.com/mathertel/OneButton
  - requires JC_Button lib https://github.com/JChristensen/JC_Button

  We are using two different button libraries for different purposes:
    - OneButton to detect a long press without detecting a short press first.
      This means the action will be evaluated on releasing the button,
      which is ok for most cases but bad for timing critical actions
      like when we are in looper mode.
    - JC_Button to detect a short press as soon as the button is pressed down.
      This button library is used in looper mode only.

  SCROLL Mode:   1/3 switches prog patches down/up
                 2 long : mode SNAPSHOT
                 1+3 long: LOOPER mode
                 
 
  SNAPSHOT Mode: 1/2/3 select snapshot 1/2/3
                 2 long : mode SCROLL
                 1+3 long: LOOPER mode
   
  LOOPER Mode:   1  record/play/overdub
                 3  play/stop undo(long)
                 2  select mode HX (presets, snapshots, footswitch)
                 3 long  toggles undo/redo
                 2 long : mode SNAPSHOT

**************************************************************************/

#include <Wire.h>
#include <OneButton.h>
#include <JC_Button.h>

#include <EEPROM.h>

// GPIO pins used
#define BTN_UP 8
#define BTN_DN 6
#define BTN_MID 7
#define LED_GRN 3
#define LED_RED 4
#define LED_BLU 5

// EEPROM addresses
#define OP_MODE_ADDR      0  // stores looper mode of operation
#define MIDI_CHANNEL_ADDR 1  // stores the midi channel 

// Adjust red LED brightness 0-255 (full on was way too bright for me)
#define LED_RED_BRIGHTNESS 130
#define LED_GRN_BRIGHTNESS 100
#define LED_BLU_BRIGHTNESS 150
// on/off delay when we flash a LED
#define LED_FLASH_DELAY    30  



OneButton btnUp(BTN_UP, true);
OneButton btnDn(BTN_DN, true);
OneButton btnMid(BTN_MID, true);

Button jc_btnUp(BTN_UP);
Button jc_btnDn(BTN_DN);
Button jc_btnMid(BTN_MID);

enum modes_t {SCROLL, SNAPSHOT, LOOPER, CHANNEL_CFG, LOOPER_CFG};       // modes of operation
static modes_t MODE;       // current mode
static modes_t LAST_MODE;  // last mode
static modes_t MODE_BEFORE_SNAPSHOT; // last mode before snap shot mode

enum lmodes_t {PLAY, RECORD, OVERDUB, STOP, NONE};   // Looper modes
static lmodes_t LPR_MODE;

uint8_t midi_channel = 0;
void (*Reset)(void) = 0;

void setup() {
  
 

  // LEDs
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GRN, OUTPUT);
  pinMode(LED_BLU, OUTPUT);

  // Buttons:
  btnUp.setClickTicks(50);
  btnUp.attachClick(upClick);
  btnUp.attachLongPressStart(upLongPressStart);

  btnDn.setClickTicks(50);
  btnDn.attachClick(dnClick);
  btnDn.attachLongPressStart(dnLongPressStart);

  btnMid.setClickTicks(50);
  btnMid.attachClick(midClick);
  btnMid.attachLongPressStart(midLongPressStart);


  // Set MIDI baud rate:
  Serial.begin(31250);



  // restore MODE from EEPROM
  MODE = (modes_t) EEPROM.read(OP_MODE_ADDR);
  if (MODE > 4){
    // no valid value in eeprom found. (Maybe this is the first power up ever?)
    MODE = SNAPSHOT;
  }
    


 // restore MIDI channel
  midi_channel = EEPROM.read(MIDI_CHANNEL_ADDR);
  if (midi_channel > 15){ 
    // set channel to 0 if data not valid
    EEPROM.update(MIDI_CHANNEL_ADDR, 0);
    midi_channel = 0;
  } 

 if (digitalRead(BTN_DN) == 0 && digitalRead(BTN_UP) == 1) {
    // btn dn pressed: configure MIDI channel
      MODE = CHANNEL_CFG;
   
    flashLED(20, LED_GRN, LED_FLASH_DELAY);
    // 'display' configure MIDI channel
    delay(1000);
    
    flashLED(midi_channel + 1, LED_GRN, 500);
  }

 

  // restore mode on HX Stomp as well
  if (MODE == SNAPSHOT){
    midiCtrlChange(71, 0); // set stomp mode
  }
  else if (MODE == LOOPER){ 
    midiCtrlChange(71, 0); // set stomp mode
  }
  else if (MODE == SCROLL) {   
    midiCtrlChange(71, 0); // set stomp mode
  }

 

  // indicate mode via LEDs
  if (MODE == LOOPER) {
//    flashRedGreen(10);
    digitalWrite(LED_GRN, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_BLU, LOW);
    flashLED(5, LED_GRN, LED_FLASH_DELAY);
     
   

    
    // we are in looper mode, so we are using the jc_button class for action on button press
    // (OneButton acts on button release)
    jc_btnUp.begin();
    jc_btnDn.begin();
    
    
  }
  else if (MODE == SCROLL){
    digitalWrite(LED_GRN, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_BLU, LOW);
    flashLED(5, LED_RED, LED_FLASH_DELAY);
  }
    
  else if (MODE == SNAPSHOT){
    digitalWrite(LED_GRN, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_BLU, LOW);
    flashLED(5, LED_BLU, LED_FLASH_DELAY);
  }
    

  // Looper default state
    LPR_MODE = NONE;

}


void loop() {

//  affichage();
  if (MODE == LOOPER) {
    
    
  
    jc_btnDn.read();                   // DN Button handled by JC_Button
    btnUp.tick();                   // Up Button handled by OneButton
    btnMid.tick();
    if (jc_btnDn.wasPressed() && digitalRead(BTN_UP) == 1)          // attach handler
      jc_dnClick();
      

  } else {
    btnUp.tick();                   // both buttons handled by OneButton
    btnDn.tick();
    btnMid.tick();
    
  }

  handle_leds();
// 
}

/* ------------------------------------------------- */
/* ---       OneButton Callback Routines          ---*/
/* ------------------------------------------------- */

void dnClick() {
  switch (MODE)
  {
    case SCROLL:
      patchDown();
      flashLED(2, LED_RED, LED_FLASH_DELAY);
      break;
    case SNAPSHOT:
      midiCtrlChange(69, 0); // snapshot 1
      flashLED(2, LED_RED, LED_FLASH_DELAY);
      break;
    case LOOPER:
      switch (LPR_MODE) {
        case NONE:
          LPR_MODE = RECORD;
          midiCtrlChange(60, 127);  // Looper record
          break;
        case STOP:
          LPR_MODE = RECORD;
          midiCtrlChange(60, 127);  // Looper record
          break;
        case RECORD:
          LPR_MODE = PLAY;
          midiCtrlChange(61, 127); // Looper play
          break;
        case PLAY:
          LPR_MODE = OVERDUB;
          midiCtrlChange(60, 0);    // Looper overdub
          break;
        case OVERDUB:
          LPR_MODE = PLAY;
          midiCtrlChange(61, 127); // Looper play
          break;
      }
      break;
    case CHANNEL_CFG:
      if(midi_channel > 0)
        midi_channel--;     
     
      flashLED(midi_channel+1, LED_GRN, 500);
      EEPROM.update(MIDI_CHANNEL_ADDR, midi_channel);    
    break;
       
  }
}
void upClick() {
  switch (MODE)
  {
    case SCROLL:
      patchUp();
      flashLED(2, LED_RED, LED_FLASH_DELAY);
      break;
    case SNAPSHOT:
      midiCtrlChange(69, 2); // Snapshot 3
      flashLED(2, LED_RED, LED_FLASH_DELAY);
      
   
      break;
    case LOOPER:
      switch (LPR_MODE) {
        case NONE:
        case STOP:
          LPR_MODE = PLAY;
          midiCtrlChange(61, 127); // Looper play
          break;
        case PLAY:
        case RECORD:
        case OVERDUB:
          LPR_MODE = STOP;
          midiCtrlChange(61, 0); // Looper stop
          break;
      }
      break;
    case CHANNEL_CFG:
      if(midi_channel < 15){
        midi_channel++;
      }
      else{
        midi_channel = 0;
      }      
     
      flashLED(midi_channel + 1, LED_GRN, 500);
      EEPROM.update(MIDI_CHANNEL_ADDR, midi_channel);
    break;
    

  }
}

void midClick() {
  switch (MODE)
  {
    case SCROLL:
      break;
    case SNAPSHOT:
      midiCtrlChange(69, 1); // Snapshot 2
      flashLED(2, LED_RED, LED_FLASH_DELAY);
      
     
      break;
    case LOOPER:
      midiCtrlChange(71, 4); // MODE Suivant
      flashLED(2, LED_RED, LED_FLASH_DELAY);
      
   
      break;
  }
  
}

void dnLongPressStart() {
  if (digitalRead(BTN_UP) == 1) {
    switch (MODE)
    {
      case SCROLL:
      while (digitalRead(BTN_DN) == 0){
        patchUp();
        flashLED(2, LED_RED, LED_FLASH_DELAY);
        delay(100);
      }
        
      case SNAPSHOT:
        break;
      case LOOPER:
        break;
    }
  }
}


void midLongPressStart() {
    switch (MODE) {
      case SCROLL:
        MODE = SNAPSHOT;
        digitalWrite(LED_GRN, LOW);
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_BLU, LOW);
        flashLED(5, LED_RED, LED_FLASH_DELAY);
        break;
      case SNAPSHOT:
        MODE = SCROLL;
        digitalWrite(LED_GRN, LOW);
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_BLU, LOW);
        flashLED(5, LED_RED, LED_FLASH_DELAY);
        break;
      case LOOPER:
        // make sure to switch off looper
        midiCtrlChange(61, 0); // Looper stop
        midiCtrlChange(71, 0); // set stomp mode
        MODE = SNAPSHOT;
        digitalWrite(LED_GRN, LOW);
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_BLU, LOW);
        flashLED(5, LED_RED, LED_FLASH_DELAY);
        break;
      case CHANNEL_CFG:
        MODE = SNAPSHOT;
        break;
    }
    EEPROM.update(OP_MODE_ADDR, SNAPSHOT);
//    // reset the device to reboot in new mode
//    Reset();
  }


  
void upLongPressStart() {
  if (digitalRead(BTN_DN) == 0) {
    // yay, both buttons pressed!
    // Toggle through modes
    switch (MODE) {
      case SCROLL:
        MODE = LOOPER;
        break;
      case LOOPER:
        midiCtrlChange(61, 0); // Looper stop
        MODE = SNAPSHOT;
        break;
      case SNAPSHOT:
        MODE = LOOPER;
        break;
      case CHANNEL_CFG:
        MODE = LAST_MODE;
      break;
    }
    EEPROM.update(OP_MODE_ADDR, MODE);
    // reset the device to reboot in new mode
    Reset();
  }
  else{
    // regular long press event:
    switch (MODE)
    {
      case SCROLL:
      while (digitalRead(BTN_UP) == 0){
        flashLED(2, LED_RED, LED_FLASH_DELAY);
        patchUp();
        delay(100);
      }
        break;
      case SNAPSHOT:
        break;
      case LOOPER:  
        switch (LPR_MODE) {
          case NONE:
          case PLAY:
          case STOP:         
            midiCtrlChange(63, 127); // looper undo/redo
            flashLED(3, LED_RED, LED_FLASH_DELAY);
            break;
          case RECORD:
          case OVERDUB:
            break;
        }

        break;
    }
  }
}

/* ------------------------------------------------- */
/* ---       JC_Button Callback Routines          ---*/
/* ------------------------------------------------- */

void jc_dnClick() {
  switch (LPR_MODE) {
    case NONE:
      LPR_MODE = RECORD;
      midiCtrlChange(60, 127);  // Looper record
      break;
    case STOP:
      LPR_MODE = RECORD;
      midiCtrlChange(60, 127);  // Looper record
      break;
    case RECORD:
      LPR_MODE = PLAY;
      midiCtrlChange(61, 127); // Looper play
      break;
    case PLAY:
      LPR_MODE = OVERDUB;
      midiCtrlChange(60, 0);    // Looper overdub
      break;
    case OVERDUB:
      LPR_MODE = PLAY;
      midiCtrlChange(61, 127); // Looper play
      break;
  }
}



/* ------------------------------------------------- */
/* ---      Midi Routines                         ---*/
/* ------------------------------------------------- */

// Use these routines if you are using firmware 3.01 or older:

// HX stomp does not have a native patch up/dn midi command
// so we are switching to scroll mode and emulating a FS1/2
// button press.
/*
void patchUp() {
  midiCtrlChange(71, 1); // HX scroll mode
  delay(30);
  midiCtrlChange(50, 127); // FS 2 (up)
  midiCtrlChange(71, 0);  // HX stomp mode
}

void patchDown() {
  midiCtrlChange(71, 1);   // HX scroll mode
  delay(30);
  midiCtrlChange(49, 127); // FS 1 (down)
  midiCtrlChange(71, 0);   // HX stomp mode
}
*/

// Added in Firmware 3.10:
//New MIDI message: CC 72 value 64-127 = next preset, value 0-63 = previous preset
void patchUp() {
  
  midiCtrlChange(72, 127); // next preset
  delay(200);

}

void patchDown() {
 
  midiCtrlChange(72, 0);   // prev preset
  delay(200);

}


void midiProgChange(uint8_t p) {
  Serial.write(0xc0 | midi_channel); // PC message
  Serial.write(p); // prog
}
void midiCtrlChange(uint8_t c, uint8_t v) {
  Serial.write(0xb0 | midi_channel); // CC message
  Serial.write(c); // controller
  Serial.write(v); // value

}

/* ------------------------------------------------- */
/* ---      Misc Stuff                            ---*/
/* ------------------------------------------------- */
void flashLED(uint8_t i, uint8_t led, uint8_t del)
{
  uint8_t last_state;
  last_state = digitalRead(led);

  for (uint8_t j = 0; j < i; j++) {
    digitalWrite(led, HIGH);
    delay(del);
    digitalWrite(led, LOW);
    delay(del);
  }
  digitalWrite(led, last_state);
}

void flashRedGreen(uint8_t i) {
  uint8_t last_state_r;
  uint8_t last_state_g;
  last_state_r = digitalRead(LED_RED);
  last_state_g = digitalRead(LED_GRN);


  for (uint8_t j = 0; j < i; j++) {
    digitalWrite(LED_RED, LOW);
    analogWrite(LED_GRN, LED_GRN_BRIGHTNESS);
    delay(75);
    analogWrite(LED_RED, LED_RED_BRIGHTNESS);
    digitalWrite(LED_GRN, LOW);
    delay(75);
  }
  digitalWrite(LED_RED, last_state_r);
  digitalWrite(LED_GRN, last_state_g);
}

void handle_leds() {
  static unsigned long next = millis();

  switch (MODE) {
    case SCROLL:
      // solid red
      digitalWrite(LED_GRN, LOW);
      digitalWrite(LED_BLU, LOW);
      analogWrite(LED_RED, LED_RED_BRIGHTNESS);
      //digitalWrite(LED_RED, HIGH);
      break;
    case SNAPSHOT:
      // solid blue
      analogWrite(LED_BLU, LED_BLU_BRIGHTNESS);
      digitalWrite(LED_GRN, LOW);
      digitalWrite(LED_RED, LOW);
      break;
    case LOOPER:
      switch (LPR_MODE) {
        case NONE:
          // violet
          analogWrite(LED_RED, LED_RED_BRIGHTNESS);
          analogWrite(LED_BLU, LED_BLU_BRIGHTNESS);
          digitalWrite(LED_GRN, LOW);
        case STOP:
          digitalWrite(LED_GRN, LOW);
          digitalWrite(LED_RED, LOW);
          digitalWrite(LED_BLU, LOW);
          break;
        case PLAY:
          analogWrite(LED_GRN, LED_GRN_BRIGHTNESS);
          digitalWrite(LED_BLU, LOW);
          digitalWrite(LED_RED, LOW);
          break;
        case RECORD:
          
           // blink red
          if (millis() - next > 500) {
            next += 500;
            digitalWrite(LED_RED, !digitalRead(LED_RED));
          }
          break;
        case OVERDUB:
          // yellow
          digitalWrite(LED_BLU, LOW);
          analogWrite(LED_GRN, LED_GRN_BRIGHTNESS);
          analogWrite(LED_RED, LED_RED_BRIGHTNESS);
          break;
      }
      
      break;
  }
}
