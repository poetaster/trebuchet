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


bool debug = true;

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
//MIDI_CREATE_DEFAULT_INSTANCE();
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI);



// define some drum kits
#define KIT 9
#define VS1003_D_BASS     35   // 35 36 86 87
#define VS1003_D_SNARE    38   // 38 40
#define VS1003_D_HHC      42   // 42 44 71 80 // good
#define VS1003_D_HHO      46   // 46 55 58 72 74 81 // long siss
#define VS1003_D_HITOM    50   // 48 50
#define VS1003_D_LOTOM    45   // 41 43 45 47
#define VS1003_D_CRASH    57   // 49 57
#define VS1003_D_RIDE     51   // 34 51 52 53 59
#define VS1003_D_TAMB     54   // 39 54
#define VS1003_D_HICONGA  62   // 60 62 65 78
#define VS1003_D_LOCONGA  64   // 61 63 64 66 79
#define VS1003_D_MARACAS  70   // 27 29 30 69 70 73 82 
#define VS1003_D_CLAVES   75   // 28 31 32 33 37 56 67 68 75 76 77 83 84 85

int kits[KIT][13] = {
  { 42, 36, 38, 42, 38, 28, 42, 31, 46, 35, 38, 46, 38 }, // clicks, side stick, snare bass
  { 35, 40, 42, 46, 35, 40, 42, 38, 42, 38, 64, 37, 42 }, // not bad
  { 36, 40, 36, 42, 42, 40, 40, 70, 46, 36, 28, 70, 37 }, // not bad
  { 35, 38, 42, 40, 35, 42, 70, 42, 46, 70, 46, 35, 42 }, // little more on the snare side/ HH, lo & hi conga, and maracas
  { 35, 38, 28, 40, 36, 46, 35, 42, 46, 38, 37, 63, 42 }, // little more on the snare side/ HH, lo conga, one clave hit
  { 46, 38, 40, 42, 27, 58, 42, 46, 44, 70, 28, 37, 64 }, // clicks snare side stick maracas
  { 44, 28, 42, 44, 28, 42, 38, 27, 44, 46, 44, 46, 85 }, // clicks, hh, castenets
  { 27, 28, 58, 28, 31, 31, 33, 34, 69, 28, 77, 31, 28 },
  { 73, 70, 31, 69, 70, 42, 82, 46, 28, 27, 69, 31, 28 },
}; // Bass, Snare, HHO, HHC

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

  long newPos = myEnc.read() / 4;
  if (newPos != kit && newPos > -1 && newPos < 9) {
    kit = newPos;
    if (debug) {
      Serial.print("kit: ");
      Serial.println(kit);
    }
    uiUpdate = true;
  }

  if (uiUpdate) {
    displayUpdate();
    uiUpdate = false;
  }

  btn_one.update();
  read_buttons();

}
// END main loop

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
      generateRandomSequence(numberOfFills, 32);
    }

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
