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
//#define INPUT_PULLUP 1
#include <Encoder.h>
Encoder myEnc(4, 3);

// the pattern generator
//euclid euc;

#define NTRACKS 3
#include "euclid.h"
int current_track = 0;
String display_pat;
bool read_lock = false;
bool encoder_select = false; // used to select fills or offset for encoder setting

struct sequencer {
  public: euclid *trigger;  // euclid object to manage hits and patterns from bastl
    uint16_t fills;   // how many hits in 16
    uint16_t repeats;   // set in euclid doStep for now, how often to repeat pattern before new
    bool  returned;  // not used, see euclid doStep
    int16_t index;    // index of step we are on
};

sequencer seq[NTRACKS] = {
  new euclid(), 4, 0, 0, NUMBER_OF_STEPS - 1, // step index
  new euclid(), 5, 0, 0, NUMBER_OF_STEPS - 1, // step index
  new euclid(), 6, 1, 0, NUMBER_OF_STEPS - 1, // step index
};



// Euclidean sequences
bool euclidianPattern[NUMBER_OF_STEPS];
uint8_t stepCounter;
uint8_t numberOfSteps;
uint8_t numberOfFills = 6;
uint8_t numberOfrepeats = 0;
char textSequence[NUMBER_OF_STEPS + 1];


// Use binary to say which MIDI channels this should respond to.
// Every "1" here enables that channel. Set all bits for all channels.
// Make sure the bit for channel 10 is set if you want drums.
//
//                               16  12  8   4  1
//                               |   |   |   |  |
uint16_t MIDI_CHANNEL_FILTER = 0b1111111111111111;


#include "pots.h"
// POTs to control which set of drums and tempo

CS_Pot TempoPot (A0, 20);
CS_Pot FillsPot (A2, 20);
CS_Pot KitPot (A1, 20);




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

// last time btn_one release, used to toggle settings in modes
unsigned long btnOneLastTime;
bool timeSigMode = false;

volatile byte tickCounter;               // Counts interrupt "ticks". Reset every 125
volatile byte fourMilliCounter;          // Counter incremented every 4ms


// Defaults, but will be overridden by POT settings if enabled
uint16_t volume = 0;
uint16_t tempo  = 60;
byte instrument;

uint8_t steps, fills, lastSteps, lastFills, repeats ;
bool swing = 0;
unsigned long nexttick;

// tempo variables
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
unsigned int numSteps = 16; // number of steps used - dependent on time signature
bool syncReceived = false; // has a sync/clock signal been received? (IMPROVE THIS LATER)

byte mode = 0; // 0 is drummer, 1 is synth.


void setup() {


  Serial.begin(57600);

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
  repeats = map(KitPot.value(), 0, 1023, 0, 16);
  numberOfFills = map(FillsPot.value(), 0, 1023, 1, 16);
  tempo =  map(TempoPot.value(), 0, 1023, 60, 240);
  randomSeed(analogRead(0));

  // try set second kit.
  //htalkMIDI(0x00,0xb0,0x7f);
  // For some reason, the last program change isn't registered
  // without an extra "dummy" read here.
  //talkMIDI(0x80, 0, 0);

  vsmidi.sendMIDI(0x00, 0xb0, 0x7f);
  vsmidi.sendMIDI(0x80, 0, 0);

  /**** set up Euclidean sequences ****/
  //generateSequence(numberOfFills, 16);

  seq[0].trigger->generateSequence(4, 16);
  seq[1].trigger->generateSequence(5, 16);
  seq[2].trigger->generateSequence(6, 16);
}

int oldKit;
int oldInst;
int oldPos = 0;
int delta = 0;

// main loop
void loop() {

  // Count every four milliseconds for adc readings
  if (tickCounter++ == 125) {
    fourMilliCounter++;
    tickCounter = 0;
  }

  // encoder has to be read in main loop.
  int newPos = myEnc.read() / 4;
  if (newPos > oldPos) {
    delta = 1;
    oldPos = newPos;
  } else if (newPos < oldPos) {
    delta = -1;
    oldPos = newPos;
  } else {
    delta = 0;
  }
  //if (debug) Serial.println(newPos);

  if (mode == 0) {
    drummerLoop();

    if (encoder_select == false) {
      int fills = seq[current_track].fills;
      if (delta != 0) {
        fills = constrain( (fills + delta), 1, 16);
        seq[current_track].fills = fills;
        seq[current_track].trigger->generateSequence(fills, 16);
        uiUpdate = true;
      }

    } else if (encoder_select == true) {
      int repeats = seq[current_track].repeats;
      if (delta != 0  ) {
        repeats = constrain( (repeats + delta), 0, 6);
        seq[current_track].repeats = repeats;
        if (repeats != 0) {
          //rotate pattern
          seq[current_track].trigger->rotate(repeats);
        } else {
          // gone to zero, so reset to base
          seq[current_track].trigger->generateSequence(seq[current_track].fills, 16);
        }
        uiUpdate = true;
      }
    }
  } else {

    synthLoop() ;
    long newPos = myEnc.read() / 4;
    if ( newPos != instrument && newPos > -1 && newPos < 101) {
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

  unsigned long timenow = millis();
  if (timenow >= nexttick) {
    //nexttick = millis() + (unsigned long)(1000 / (tempo / 60) ) / 24;
    nexttick = millis() + (unsigned long)(60000 / tempo ) / 4; // 12 for triplets or 4 for 1/4 16th

    for (uint8_t track = 0; track < NTRACKS; ++track) { // note that triggers are stored MSB first
      if ( seq[track].trigger->getCurrentStep() ) {

        //  { 35, 36, 35, 38, 40, 38, 46, 28, 42, 46, 31 }, kits are 3s of bass section, snare section, hat section

        int offset = track * 3;
        int rr = random(3) + offset; // (0,3 3,6, 6, 9)
        int note = kits[kit][rr];

        // we're addding a bit of randomness to velocity
        int vel = 127 - random(55);
        if (note == 75 || note == 57 || note == 67 || note == 31) {
          vel = vel - 30 ;
        }
        vsmidi.noteOn( 9, note, vel); // channel 9 is 10 in midi.
      } else {

      }
      seq[track].trigger->doStep(); // next step advance

    }

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

  if (btn_one.rose()) {
    int btnOneLastTime = btn_one.previousDuration();
    if (btnOneLastTime > 500) {
      mode = !mode;
      if (mode == 1) {
        vsmidi.allNotesOff();
        // start input
        MIDI.begin(MIDI_CHANNEL_OMNI);

        // update display
        const char* instrumentName = (const char*)pgm_read_ptr(&gmInstruments[0]);
        strncpy_P(buffer, instrumentName, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        synthDisplayUpdate();

      } else {
        //MIDI.stop();
        vsmidi.allNotesOff();
        uiUpdate = true;
      }
    }
  }

  // if button one was held for more than 75 millis
  //int debouncedState =  btn_one.read();
  if (btn_one.released() && btn_one.currentDuration() < 500) {
    encoder_select = !encoder_select;
  }
}

// read pots
void adc() {
  if (read_lock == false) {

    int pot0 = map(KitPot.value(), 5, 1014, 0, 8);
    int pot3 = map(FillsPot.value(), 5, 1014, 0, 2);

    // must ensure pots return to last value
    if (pot0 != kit) {
      kit = pot0;
      uiUpdate = true;
    }
    if (pot3 != current_track) {
      current_track = pot3;
      uiUpdate = true;
    }

    int pot1 = map(TempoPot.value(), 5, 1014, 40, 320);
    if (pot1 != tempo) {
      tempo = pot1;  // Tempo range is 20 to 275.
      uiUpdate = true;
    }
  }
}
