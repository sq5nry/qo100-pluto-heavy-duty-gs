/*
  Based on Sketch built by Gustavo Silveira (aka Music Nerd)
  Modified by Dolce Wang

  This code is only for Arduinos that use ATmega32u4 (like Micro, Pro Micro, Leonardo...)
  Remember to also assign the correct board in the IDE (like Tools / Boards / Sparkfun AVR / Pro Micro...)

Updates from SQ5NRY for SDR Console control:
- single PTT button
- single pot on A0
*/

// Change any fields with //**


// LIBRARY

#include "MIDIUSB.h"  

// BUTTONS
const int NButtons = 1; //***  total number of buttons
const int buttonPin[NButtons] = {20}; //*** define Digital Pins connected from Buttons to Arduino; (ie {10, 16, 14, 15, 6, 7, 8, 9, 2, 3, 4, 5}; 12 buttons)
                                            
int buttonCState[NButtons] = {};        // stores the button current value
int buttonPState[NButtons] = {};        // stores the button previous value

      
// debounce
unsigned long lastDebounceTime[NButtons] = {0};  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    //** the debounce time; increase if the output flickers


// POTENTIOMETERS
const int NPots = 1; //*** total number of pots (knobs and faders)
const int potPin[NPots] = {A0}; //*** define Analog Pins connected from Pots to Arduino; Leave nothing in the array if 0 pots {}
int potCState[NPots] = {0}; // Current state of the pot; delete 0 if 0 pots
int potPState[NPots] = {0}; // Previous state of the pot; delete 0 if 0 pots
int potVar = 0; // Difference between the current and previous state of the pot

int midiCState[NPots] = {0}; // Current state of the midi value; delete 0 if 0 pots
int midiPState[NPots] = {0}; // Previous state of the midi value; delete 0 if 0 pots

const int TIMEOUT = 300; //* Amount of time the potentiometer will be read after it exceeds the varThreshold
const int varThreshold = 10; //* Threshold for the potentiometer signal variation
boolean potMoving = true; // If the potentiometer is moving
unsigned long PTime[NPots] = {0}; // Previously stored time; delete 0 if 0 pots
unsigned long timer[NPots] = {0}; // Stores the time that has elapsed since the timer was reset; delete 0 if 0 pots


// MIDI Assignments 
byte midiCh = 1; //* MIDI channel to be used
byte note = 36; //* Lowest note to be used; 36 = C2; 60 = Middle C
byte cc = 1; //* Lowest MIDI CC to be used


// SETUP
void setup() {

  // Baud Rate
  // 31250 for MIDI class compliant | 115200 for Hairless MIDI

  // Buttons
  // Initialize buttons with pull up resistors
  for (int i = 0; i < NButtons; i++) {
    pinMode(buttonPin[i], INPUT_PULLUP);
  }

}

////
// LOOP
void loop() {

  buttons();
  potentiometers();

}

////
// BUTTONS
void buttons() {

  for (int i = 0; i < NButtons; i++) {

    buttonCState[i] = digitalRead(buttonPin[i]);  // read pins from arduino

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {

      if (buttonPState[i] != buttonCState[i]) {
        lastDebounceTime[i] = millis();

        if (buttonCState[i] == LOW) {

          // Sends the MIDI note ON 
          
         // use if using with ATmega32U4 (micro, pro micro, leonardo...)
          noteOn(midiCh, note + i, 127);  // channel, note, velocity
          MidiUSB.flush();


        }
        else {

          // Sends the MIDI note OFF accordingly to the chosen board

          // use if using with ATmega32U4 (micro, pro micro, leonardo...)
          noteOn(midiCh, note + i, 0);  // channel, note, velocity
          MidiUSB.flush();

        }
        buttonPState[i] = buttonCState[i];
      }
    }
  }
}

////
// POTENTIOMETERS
void potentiometers() {


  for (int i = 0; i < NPots; i++) { // Loops through all the potentiometers

    potCState[i] = analogRead(potPin[i]); // reads the pins from arduino

    midiCState[i] = map(potCState[i], 0, 1023, 0, 127); // Maps the reading of the potCState to a value usable in midi

    potVar = abs(potCState[i] - potPState[i]); // Calculates the absolute value between the difference between the current and previous state of the pot

    if (potVar > varThreshold) { // Opens the gate if the potentiometer variation is greater than the threshold
      PTime[i] = millis(); // Stores the previous time
    }

    timer[i] = millis() - PTime[i]; // Resets the timer 11000 - 11000 = 0ms

    if (timer[i] < TIMEOUT) { // If the timer is less than the maximum allowed time it means that the potentiometer is still moving
      potMoving = true;
    }
    else {
      potMoving = false;
    }

    if (potMoving == true) { // If the potentiometer is still moving, send the change control
      if (midiPState[i] != midiCState[i]) {

        // Sends  MIDI CC 
        // Use if using with ATmega32U4 (micro, pro micro, leonardo...)
        controlChange(midiCh, cc + i, midiCState[i]); //  (channel, CC number,  CC value)
        MidiUSB.flush();

        potPState[i] = potCState[i]; // Stores the current reading of the potentiometer to compare with the next
        midiPState[i] = midiCState[i];
      }
    }
  }
}


// if using with ATmega32U4 (micro, pro micro, leonardo...)


// Arduino MIDI functions MIDIUSB Library
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}
