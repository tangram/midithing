// Copyright 2014 Sergio Retamero.
//
// Author: Sergio Retamero (sergio.retamero@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//

#include "firmware.h"
#include "Blinker.h"
#include "MIDIClass.h"
#include "MultiPointConv.h"


// Variables
// Var I2C DAC
mcp4728 Dac = mcp4728(0); // instantiate mcp4728 object, Device ID = 0
MultiPointConv DACConv[4];

// Var Learn Mode
byte LearnMode = NORMALMODE;
unsigned long LearnInitTime;
byte LearnStep = 0;

// Var Blinker
Blinker Blink((byte)PINLED);
Blinker Gates[10];

Bounce Bouncer = Bounce(); // will be configured in setup()
unsigned long BouncerLastTime = 0;

struct MIDISettings : public midi::DefaultSettings {
  static const bool UseRunningStatus = false;
  static const bool Use1ByteParsing = false;
};

MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, MIDISettings);


// Initialization
void setup()
{
  // Configure digital outputs
  pinMode(PINLED, OUTPUT);
  digitalWrite(PINLED, LOW); // Set internal pull down resistor
  pinMode(PINLED2, OUTPUT);
  digitalWrite(PINLED2, LOW); // Set internal pull down resistor
  pinMode(PINGATE, OUTPUT);
  digitalWrite(PINGATE, LOW); // Set internal pull down resistor
  pinMode(PINGATE2, OUTPUT);
  digitalWrite(PINGATE2, LOW); // Set internal pull down resistor
  pinMode(PINGATE3, OUTPUT);
  digitalWrite(PINGATE3, LOW); // Set internal pull down resistor
  pinMode(PINGATE4, OUTPUT);
  digitalWrite(PINGATE4, LOW); // Set internal pull down resistor
  pinMode(PINSTARTSTOP, OUTPUT);
  digitalWrite(PINSTARTSTOP, LOW); // Set internal pull down resistor
  pinMode(PINCLOCK, OUTPUT);
  digitalWrite(PINCLOCK, LOW); // Set internal pull down resistor

  pinMode(PINLEARN, INPUT); // maybe INPUT_PULLUP?
  // Attach the Bounce object with a 50 millisecond debounce time
  Bouncer.attach(PINLEARN);
  Bouncer.interval(50);
  //digitalWrite(PINLEARN, LOW); // Set internal pull down resistor

  // DAC MCP4728 init
  Dac.begin();  // initialize i2c interface
  Dac.vdd(5000); // set VDD(mV) of MCP4728 for correct conversion between LSB and Vout
  Dac.setVref(1, 1, 1, 1); // Use internal vref
  Dac.setGain(1, 1, 1, 1); // Use gain x2
  for (int i = 0; i < 4; i++) {
    DACConv[i].DACnum = i;
  }

  // Init triggers gates
  Gates[0].pinLED = PITCHCV + 128;
  Gates[1].pinLED = VELOC + 128;
  Gates[2].pinLED = MODUL + 128;
  Gates[3].pinLED = BEND + 128;
  Gates[4].pinLED = PINCLOCK;
  Gates[5].pinLED = PINGATE;
  Gates[6].pinLED = PINGATE2;
  Gates[7].pinLED = PINGATE3;
  Gates[8].pinLED = PINGATE4;
  Gates[9].pinLED = PINSTARTSTOP;

  //  Init MIDI:
  //Serial.begin(115200);
  MIDI.turnThruOff(); // Thru off disconnected
  MIDI.setHandleNoteOn(HandleNoteOn); // Handle for NoteOn
  MIDI.setHandleNoteOff(HandleNoteOff); // Handle for NoteOff
  MIDI.setHandlePitchBend(HandlePitchBend); // Handle for Pitch Bend
  MIDI.setHandleControlChange(HandleControlChange); // Handle for CC
  #ifdef STARTSTOPCONT
  MIDI.setHandleStart(HandleStart); // Handle Start, Continue, Stop
  MIDI.setHandleContinue(HandleContinue);
  MIDI.setHandleStop(HandleStop);
  #endif
  MIDI.setHandleClock(HandleClock);
  MIDI.begin(MIDI_CHANNEL_OMNI); // Listen to all channels

  #ifdef PRINTDEBUG
  Serial.println("Init MIDItoCV...");
  #endif

  // Read MIDI Channels from EEPROM and store
  if (ReadMIDIeeprom() == -1) {
    #ifdef PRINTDEBUG
    Serial.println("No EEPROM Read");
    #endif
    // Set Mode manually
    //SetVoiceMode(MONOMIDI);
    //SetVoiceMode(DUALMIDI);
    //SetVoiceMode(QUADMIDI);
    //SetVoiceMode(POLYFIRST);
    //SetVoiceMode(PERCTRIG);
    SetVoiceMode(DUOLOW);
  }

  // LDAC pin must be grounded for normal operation.
  // Reset DAC values to 0 after reset
  delay(50);
  Dac.analogWrite(0, 0, 0, 0);

  #ifdef CALIBRATION
  analogReference(INTERNAL);
  #endif
  // LIGHTSHOW TIME!!
  Blink.setBlink(50, 50, 3, PINLED);
  delay(200);
  Blink.setBlink(0, 0, 0);
  Blink.setBlink(50, 50, 3, PINLED2);
  delay(200);
  Blink.setBlink(0, 0, 0);
  Blink.setBlink(50, 50, 3, PINGATE);
  delay(200);
  Blink.setBlink(0, 0, 0);
  Blink.setBlink(50, 50, 3, PINGATE2);
  delay(200);
  Blink.setBlink(0, 0, 0);
  Blink.setBlink(50, 50, 3, PINGATE3);
  delay(200);
  Blink.setBlink(0, 0, 0);
  Blink.setBlink(50, 50, 3, PINGATE4);
  delay(200);
  Blink.setBlink(0, 0, 0);
  Blink.setBlink(50, 50, 3, PINCLOCK);
  delay(200);
  /*
  Blink.setBlink(0, 0, 0);
  Blink.setBlink(50, 50, 3, PINSTARTSTOP);
  delay(200);
   */
  Blink.setBlink(0, 0, 0);

}


// Main Loop
void loop()
{
  // put your main code here, to run repeatedly:

  MIDI.read();

  // handle blinks
  Blink.playBlink();
  for (int i = 0; i < 10; i++) {
    Gates[i].playBlink();
  }

  // Cal/Learn Button
  if (Bouncer.update()) { // button state changed
    unsigned long now = millis();
    unsigned long bouncerDuration = now - BouncerLastTime;

    BouncerLastTime = now;

    // Check for learn/cal mode signal
    if (Bouncer.fell()) {
      // If button pressed during calbration, end calibration
      if (LearnMode == ENTERCAL) {
        EndCalMode();
      }
      else if (LearnMode == ENTERLEARN) {
        CancelLearnMode();
      }
      // Enter Calmode after 5 secs button press
      else if (bouncerDuration > 5000) {
        EnterCalMode();
      }
      // More than one second: Learn Mode
      else if (bouncerDuration > 1000 && VoiceMode != PERCTRIG && VoiceMode != PERCGATE ) {
        EnterLearnMode();
      }
      // Panic
      else if (bouncerDuration > 100) {
        AllNotesOff();
      }
    }
  } else { // no change to the button state
    // In Learn mode, enter learn cycle
    if (LearnMode == ENTERLEARN) {
      DoLearnCycle();
    } else if (LearnMode == ENTERCAL) {
      DoCalCycle();
    }
  }
}


// DAC function definition
// Send value val to DAC port
void  SendvaltoDAC(unsigned int port, unsigned int val)
{
  Dac.analogWrite(port, val); // write to input register of a DAC. Channel 0-3, Value 0-4095

  #ifdef PRINTDEBUG
  Serial.print(port);
  Serial.print(" DAC = ");
  Serial.println(val);
  #endif
}


void BlinkOK(void) {
  // BLINK OK
  Blink.setBlink(0, 0, 0);
  Blink.setBlink(100, 1, 1, PINLED);
  delay(200);
  Blink.setBlink(0, 0, 0, PINLED);
  Blink.setBlink(100, 1, 1, PINLED2);
  delay(200);
  Blink.setBlink(0, 0, 0, PINLED2);
}

void BlinkKO(void) {
  // XXX BLINK ERROR
  Blink.setBlink(0, 0, 0);
  Blink.setBlink(0, 0, 0, PINLED);
  Blink.setBlink(100, 1, 1, PINSTARTSTOP);
  delay(200);
  Blink.setBlink(0, 0, 0);
  Blink.setBlink(100, 1, 1, PINCLOCK);
  delay(200);
  Blink.setBlink(0, 0, 0);
}

