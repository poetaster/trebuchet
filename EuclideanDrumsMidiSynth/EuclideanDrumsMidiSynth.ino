/*

  Euclidean Midi Drums
  Copyright (c) 2024 blueprint@poetaster.de (Mark Washeim)
  some parts Copyright (c) 2020 diyelectromusic (Kevin)

  GPLv3 See LICENSE distributed with this file.

  Much of the code and inspiration is from Kevin
  Simple DIY Electronic Music Projects - https://diyelectromusic.wordpress.com

  The euclidean rythm code is from https://bastl-instruments.com/ the one chip modules
  https://github.com/bastl-instruments/one-chip-modules

  This uses a generic VS1003 or VS1053 MP3 Player board utilising
  VS10xx software patching to provide a robust "real time MIDI" mode
  with MIDI access provided over the SPI bus.

*/
using namespace std;
#include <SPI.h>
#include <Wire.h>
#include <MIDI.h>
#include <VS1053_MIDI.h>

// button inputs
#define SW1 2
#include <Bounce2.h>
Bounce2::Button btn_one = Bounce2::Button();

#include <ss_oled.h>

SSOLED ssoled;
#define SDA_PIN A4
#define SCL_PIN A5
// no reset pin needed
#define RESET_PIN -1
// let ss_oled find the address of our display
#define OLED_ADDR -1
#define FLIP180 1
#define INVERT 0
// Use the default Wire library
#define USE_HW_I2C 1
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32


bool debug = false;

// init encoder, are on 3, 4 button on 2
// this lib does accel and counts multiple clicks.

//#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <Encoder.h>
Encoder myEnc(4, 3);

// the pattern generator
//euclid euc;
// move back to object ASAP
#include "euclid.h"

#define NUMBER_OF_STEPS 32

// Euclidean sequences
bool euclidianPattern[NUMBER_OF_STEPS + 1];
uint8_t stepCounter;
uint8_t numberOfSteps;
uint8_t numberOfFills = 6;
uint8_t numberOfrepeats = 0;



// Use binary to say which MIDI channels this should respond to.
// Every "1" here enables that channel. Set all bits for all channels.
// Make sure the bit for channel 10 is set if you want drums.
//
//                               16  12  8   4  1
//                               |   |   |   |  |
uint16_t MIDI_CHANNEL_FILTER = 0b1111111111111111;

// Comment out any of these if not in use
//#define POT_MIDI  A0 // MIDI control
//#define POT_VOL   A3 // Volume control

#include "pots.h"
// POTs to control which set of drums and tempo
CS_Pot KitPot (A2, 20);
CS_Pot TempoPot (A0, 20);
CS_Pot FillsPot (A1, 20);

//#define POT_REPEATS A3




// Channel to link to the potentiometer (1 to 16)
#define POT_MIDI_CHANNEL 10

// VS1053 Shield pin definitions
#define VS_MOSI   11
#define VS_MISO   12
#define VS_SCK    13
#define VS_SS     10

#define VS1053_XCS   9 // Control Chip Select Pin (for accessing SPI Control/Status registers)
#define VS1053_XDCS  7 // 7 Data Chip Select / BSYNC Pin
#define VS1053_DREQ  6 // 2 Data Request Pin: Player asks for more data
#define VS1053_RESET 8 // 8 Reset is active low

// Create VS1053_MIDI instance
VS1053_MIDI vsmidi(VS1053_XCS, VS1053_XDCS, VS1053_DREQ, VS1053_RESET);

// There are three selectable sound banks on the VS1053
// These can be selected using the MIDI command 0xBn 0x00 bank
#define DEFAULT_SOUND_BANK 0x00  // General MIDI 1 sound bank
#define DRUM_SOUND_BANK    0x78  // Drums
#define ISNTR_SOUND_BANK   0x79  // General MIDI 2 sound bank 0x7f


// This is required to set up the MIDI library.
// The default MIDI setup uses the built-in serial port.
MIDI_CREATE_DEFAULT_INSTANCE();
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI);

#include "kits.h"

// buffer for GM names display
char buffer[32];
#include "GMnames.h"

int kit = 0;



// Globals, clean up!


volatile byte tickCounter;               // Counts interrupt "ticks". Reset every 125
volatile byte fourMilliCounter;          // Counter incremented every 4ms


// Defaults, but will be overridden by POT settings if enabled
uint16_t volume = 0;
uint16_t tempo  = 60;
byte instrument;

uint8_t steps, fills, lastSteps, lastFills, repeats ;
uint8_t currentRepeat = 0;
bool bState1, bState2;
bool swing = 0;
uint8_t clkCounter = 0;
uint8_t instrs;
uint8_t timings;
int loopstate;
unsigned long nexttick;

// tempo variables
float tapTimes[4];
byte nextTapIndex = 0;
byte paramTimeSignature = 64; // 1 to 13 (3/4, 4/4, 5/4, 6/4, 7/4)
float paramTempo = 80;
byte paramSwing = 0; // 0 to 2 (0=straight, 1=approx quintuplet swing, 2=triplet swing)

int syncTicks = 0;
unsigned long synctick;
unsigned long synctick2;
unsigned long sync;

// forward declares
#include "display.h"

// time keeping global variables much from drum kid
float nextPulseTime = 0.0; // the time, in milliseconds, of the next pulse
float msPerPulse = 20.8333; // time for one "pulse" (there are 24 pulses per quarter note, as defined in MIDI spec)
byte pulseNum = 0; // 0 to 23 (24ppqn, pulses per quarter note)
unsigned int stepNum = 0; // 0 to 192 (max two bars of 8 beats, aka 192 pulses)
unsigned int numSteps = 32; // number of steps used - dependent on time signature
bool syncReceived = false; // has a sync/clock signal been received? (IMPROVE THIS LATER)

byte mode = 0; // 0 is drummer, 1 is synth.


void setup() {

  if (debug == false) {
    //blows up sigh
    //MIDI.begin(MIDI_CHANNEL_OMNI);//MIDI_CHANNEL_OMNI);
  } else {
    Serial.begin(57600);
  }
  // Initialize SPI
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);

  // Initialize VS1053 with MIDI plugin
  if (vsmidi.begin(true)) {
    if (debug) {
      Serial.println("✓ VS1053_MIDI initialized successfully!");
      Serial.print("Plugin size: ");
      Serial.print(vsmidi.getPluginSize());
      Serial.println(" words");
      Serial.println();
    }
  } else {
    if (debug) {
      Serial.println("✗ VS1053_MIDI initialization failed!");
    }
    while (1);
  }

  // Set volume
  vsmidi.setMasterVolume(0x00, 0x00);
  // setup display
  displaySetup();

  // setup the encoder button
  btn_one.attach( SW1 , INPUT_PULLUP);
  btn_one.interval(5);
  btn_one.setPressedState(LOW);

  //setup pot objects
  KitPot.begin();
  TempoPot.begin();
  FillsPot.begin();
  delay(200);

  // initialize from current pot positoins
  repeats = map(KitPot.value(), 0, 1023, 0, 32);
  numberOfFills = map(FillsPot.value(), 0, 1023, 1, 32);
  tempo =  map(TempoPot.value(), 0, 1023, 60, 240);


  // try set second kit.
  //htalkMIDI(0x00,0xb0,0x7f);
  // For some reason, the last program change isn't registered
  // without an extra "dummy" read here.
  //talkMIDI(0x80, 0, 0);

  vsmidi.sendMIDI(0x00, 0xb0, 0x7f);
  vsmidi.sendMIDI(0x80, 0, 0);

  /**** set up Euclidean sequences ****/
  generateSequence(numberOfFills, 32);
}


// main loop
void loop() {

  // Count every four milliseconds for adc readings
  if (tickCounter++ == 125) {
    fourMilliCounter++;
    tickCounter = 0;
  }

  // encoder has to be read in main loop.
  long newPos = myEnc.read() / 4;
  
  if (mode == 0) {
    drummerLoop();
    
    if (newPos != kit && newPos > -1 && newPos < 9) {
      kit = newPos;
      if (debug) {
        Serial.print("kit: ");
        Serial.println(kit);
      }
      uiUpdate = true;
    }
    
  } else {
  
    synthLoop() ;
    long newPos = myEnc.read() / 4;
    if (newPos != instrument && newPos > -1 && newPos < 128) {
      instrument = newPos;
      vsmidi.sendMIDI(0xC0 | 0, instrument, 0);
      synthDisplayUpdate();
    }

  }


  btn_one.update();
  read_buttons();

}

// END main loop



// LOOP drummer mode
void drummerLoop() {

  msPerPulse = 2500.0 / tempo ; // 20.833 ms per beat at 4/4 120BPM
  unsigned long timenow = millis();
  if (timenow >= nexttick) {
    //nexttick = sync + millis();
    // we use a multiplier of parts of fill in 32 steps.
    // this should be adjustable

    nexttick = millis() + (unsigned long)(1000 / (tempo / 60) ) / 24;
    //nexttick = nexttick + msPerPulse;

    //Serial.println(nexttick);
    // check if we've hit the end of a 32 step pattern
    // if so, new pattern. reset counter.

    if ( clkCounter == 32) {
      if (currentRepeat == repeats) {
        generateSequence(numberOfFills, 32);
        clkCounter = 0;
        currentRepeat = 0;
        Serial.println("new pattern");

      }
    }
    // set next repeat counter on pattern end
    if (clkCounter == 32) {
      currentRepeat++;
      Serial.print("incr repeat: ");
      Serial.println(currentRepeat);
    }
    //Serial.println(getStep(stepCounter));

    // update / play sound if currentstep is on
    if (getCurrentStep() ) {
      //Serial.println(getStepNumber());
      int rr = random(13); // should use length of [kit]
      int note = kits[kit][rr];
      //if (debug) Serial.println ( note);

      // we're addding a bit of randomness to velocity
      int vel = 127 - random(70);
      if (note == 75 || note == 57) {
        vel = 90 ;
      }
      vsmidi.noteOn( 9, note, vel); // channel 9 is 10 in midi.
      //vsmidi.sendMIDIPacket(0x90 | 10, note, vel, true);
      //vsmidi.sendMIDI(0x99, note, vel);

      //swing a bit but not too much
      if (swing) {
        delay (random(3));
        //Serial.println("swinging");
      }

      //talkMIDI (0x89, note, 0);
      //delay (50);
    }
    // generate the current step 0/1, increment counter
    doStep();
    clkCounter++;
  }

  // Check controls every 1/10 second adc readings.
  if (fourMilliCounter > 5) {
    fourMilliCounter = 0;
    adc();
  }

  if (uiUpdate) {
    drumsDisplayUpdate();
    uiUpdate = false;
  }

}
// END drummer loop

// LOOP synth

void synthLoop() {
  // put your main code here, to run repeatedly:
  // Read instrument name from PROGMEM

  const char* instrumentName = (const char*)pgm_read_ptr(&gmInstruments[instrument - 1]);
  strncpy_P(buffer, instrumentName, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';

  // If we have MIDI data then forward it on.
  // Based on the DualMerger.ino example from the MIDI library.
  //
  if (MIDI.read()) {
    // Extract the channel and type to build the command to pass on
    // Recall MIDI channels are in the range 1 to 16, but will be
    // encoded as 0 to 15.
    //
    // All commands in the range 0x80 to 0xE0 support a channel.
    // Any command 0xF0 or above do not (they are system messages).
    //
    byte ch = MIDI.getChannel();
    uint16_t ch_filter = 1 << (ch - 1); // bit numbers are 0 to 15; channels are 1 to 16
    if (ch == 0) ch_filter = 0xffff; // special case - always pass system messages where ch==0
    if (MIDI_CHANNEL_FILTER & ch_filter) {
      byte cmd = MIDI.getType();
      if ((cmd >= 0x80) && (cmd <= 0xE0)) {
        // Merge in the channel number
        cmd |= (ch - 1);
      }
      vsmidi.sendMIDI(cmd, MIDI.getData1(), MIDI.getData2());
    }
  }
}
// END synth loop

void read_buttons() {
  bool doublePressMode = false;
  // if button one was held for more than 75 millis
  /*
    if (btn_one.rose()) {
    btnOneLastTime = btn_one.previousDuration();
    if (btnOneLastTime > 250 && voice_number == 1) easterEgg = !easterEgg;
    }*/

  if (!doublePressMode) {
    // being tripple shure :)
    if (btn_one.pressed() ) {
      mode = !mode;
      //generateRandomSequence(numberOfFills, 32);
      if (mode == 1) {
        vsmidi.allNotesOff();
        // start input
        MIDI.begin(MIDI_CHANNEL_OMNI);
        const char* instrumentName = (const char*)pgm_read_ptr(&gmInstruments[0]);
        strncpy_P(buffer, instrumentName, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        synthDisplayUpdate();
      } else {
        //MIDI.stop();
        vsmidi.allNotesOff();
      }
    }

  }
}

// read pots
void adc() {

  int pot0 = map(KitPot.value(), 0, 1023, 0, 31);
  if (pot0 != repeats) {
    repeats = pot0;
    currentRepeat = 0;
    if (debug) {
      Serial.print ("Repeats: ");
      Serial.println ( repeats);
    }
    uiUpdate = true;
  }
  int pot3 = map(FillsPot.value(), 0, 1023, 1, 32);
  if (pot3 != numberOfFills) {
    numberOfFills = pot3;  // Number of fills 4-16
    // reset repeats
    if (debug) {
      Serial.print ("Fills: ");
      Serial.println(numberOfFills);
    }
    currentRepeat = 0;
    clkCounter = 0;
    generateSequence(numberOfFills, 32);
    uiUpdate = true;

  }
  int pot1 = map(TempoPot.value(), 0, 1023, 60, 240);
  if (pot1 != tempo) {
    tempo = pot1;  // Tempo range is 20 to 275.
    currentRepeat = 0;
    clkCounter = 0;
    if (debug) {
      Serial.print ("Tempo: ");
      Serial.println ( tempo);
    }
    uiUpdate = true;
  }
}

/**** Euclidean rythm algos ****/

uint8_t getStepNumber() {
  return stepCounter;
};
uint8_t getNumberOfFills() {
  return numberOfFills;
};

void generateSequence( uint8_t fills, uint8_t steps) {
  numberOfSteps = steps;
  if (fills > steps) fills = steps;
  if (fills <= steps) {
    for (int i = 0; i < 32; i++) euclidianPattern[i] = false;
    if (fills != 0) {
      euclidianPattern[0] = true;
      float coordinate = (float)steps / (float)fills;
      float whereFloat = 0;
      while (whereFloat < steps) {
        uint8_t where = (int)whereFloat;
        if ((whereFloat - where) >= 0.5) where++;
        euclidianPattern[where] = true;
        whereFloat += coordinate;
      }
    }
  }
}

void generateRandomSequence( uint8_t fills, uint8_t steps) {
  //if(numberOfSteps!=steps && numberOfFills!=fills){
  numberOfSteps = steps;
  numberOfFills = fills;
  if (fills > steps) fills = steps;
  if (fills <= steps) {
    for (int i = 0; i < 32; i++) euclidianPattern[i] = false;
    //euclidianPattern[17]=true;
    if (fills != 0) {
      //  euclidianPattern[0]=true;
      //Serial.println();
      for (int i = 0; i < fills; i++) {
        uint8_t where;
        //if(euclidianPattern[where]==false) euclidianPattern[where]=true;//, Serial.print(where);
        //else{
        while (1) {
          where = random(steps);
          if (euclidianPattern[where] == false) {
            euclidianPattern[where] = true; // if (debug) (Serial.print(where),Serial.print(" "));
            break;
          }

        }
        //}
      }
      //if (debug) Serial.println();
    }
  }
  //}
}


void rotate(uint8_t _steps) {
  for (int i = 0; i < _steps; i++) {
    bool temp = euclidianPattern[numberOfSteps];
    for (int j = numberOfSteps; j > 0; j--) {
      euclidianPattern[j] = euclidianPattern[j - 1];
    }
    euclidianPattern[0] = temp;
  }
}
bool getStep(uint8_t _step) {
  return euclidianPattern[_step];
}
bool getCurrentStep() {
  return euclidianPattern[stepCounter];
}
void doStep() {
  if (stepCounter < (numberOfSteps - 1)) stepCounter++;
  else stepCounter = 0;
}

void resetSequence() {
  stepCounter = 0;
}
