/* EEPROM  mostly from modulove A-RYTH-MATIK
  { fills } { offsets } Kit, Tempo, Instument, ExtClock
  // Define only the first 10 presets for LGT8FX_BOARD
*/
// we save preset & slot before this
#define EEPROM_START_ADDRESS 60
#define NUM_PRESETS 10
#define CONFIG_SIZE (sizeof(ConfigSlot))
#define LAST_USED_SLOT_ADDRESS (EEPROM_START_ADDRESS + NUM_PRESETS * CONFIG_SIZE)


// we have 10 preset progmem and save altered/new settings to eeprom slots

struct ConfigSlot {
  byte fills[3];         // Number of hits per pattern in each channel
  byte offset[3];       // Step offset for each channel
  byte limit[3];        // Step limit (length) of the pattern for each channel
  byte probability[3];  // Probability of triggering each hit (0-100%)
  int kit;              // byte of kit, 0-8
  int tempo;                       // Tempo for the preset
  int GMinst ;                      // midi inst
  bool internalClock;              // Clock source state
  byte lastUsedSlot;               // Last used slot
  byte selectedPreset;             // last used preset / slot > 10 is an eeprom slot
};

const ConfigSlot defaultSlots[NUM_PRESETS] PROGMEM = {
  { { 4, 5, 4 }, { 0, 1, 2 }, {16, 16, 16}, {100, 100, 100}, 3, 112, 1, 0 , 0, 0 },
  { { 2, 5, 3 }, { 0, 0, 2 }, {16, 16, 16}, {100, 100, 100}, 0, 115, 1, 0 , 0, 0 },
  { { 4, 4, 4 }, { 0, 1, 1 }, {16, 16, 16}, {100, 100, 100}, 8, 150, 1, 0 , 0, 0 },
  { { 3, 4, 4 }, { 0, 2, 0 }, {16, 16, 16}, {100, 100, 100}, 7, 133, 1, 0 , 0, 0 },
  { { 6, 6, 3 }, { 2, 1, 2 }, {16, 16, 16}, {100, 100, 100}, 6, 104, 1, 0 , 0, 0 },
  { { 4, 6, 3 }, { 3, 1, 1 }, {16, 16, 16}, {100, 100, 100}, 5, 110, 1, 0 , 0, 0 },
  { { 4, 5, 3 }, { 1, 2, 2 }, {16, 16, 16}, {100, 100, 100}, 4, 110, 1, 0 , 0, 0 },
  { { 2, 4, 6 }, { 2, 1, 1 }, {16, 16, 16}, {100, 100, 100}, 3, 141, 1, 0 , 0, 0 },
  { { 4, 5, 4 }, { 2, 1, 3 }, {16, 16, 16}, {100, 100, 100}, 2, 121, 1, 0 , 0, 0 },
  { { 6, 5, 3 }, { 0, 1, 2 }, {16, 16, 16}, {100, 100, 100}, 1, 95, 1, 0 , 0, 0 },
};

ConfigSlot memorySlots[NUM_PRESETS], currentConfig;

byte lastUsedSlot = 0;
bool offset_buf[3][16];  //offset buffer , Stores the offset result

// selected_preset = (selected_preset + increment + sizeof(defaultSlots) / sizeof(ConfigSlot)) % (sizeof(defaultSlots) / sizeof(ConfigSlot));

// just for reference we calculate and don't store the patterns
void updateRythm() {
  for (uint8_t i = 0; i < 3; ++i) {
    uint8_t hits = currentConfig.fills[i];
    uint8_t offset = currentConfig.offset[i];
    uint8_t limit = currentConfig.limit[i];
    seq[i].fills = hits;
    seq[i].repeats = offset;
    seq[i].trigger->generateSequence(hits, limit);
    if (offset != 0) seq[i].trigger->rotate(offset);
    
  }
  if (debug) Serial.println("updated patterns");
  uiUpdate = true;
  
}

// Loading ConfigSlot from PROGMEM
void loadDefaultConfig(ConfigSlot *config, int index) {
  if (index >= NUM_PRESETS) index = 0;
  memcpy_P(config, &defaultSlots[index], sizeof(ConfigSlot));
  //disp_refresh = true;
  updateRythm();
  mode = 0; // return to drum interface
}

void loadFromPreset(int preset) {
  if (preset >= sizeof(defaultSlots) / sizeof(ConfigSlot)) preset = 0;
  loadDefaultConfig(&currentConfig, preset);
  tempo = currentConfig.tempo;
  kit = currentConfig.kit;
  internalClock = currentConfig.internalClock;
  selected_preset = preset;
  //period = 60000 / tempo / 4;
  setup_complete = true;
}

// track of current preset and slot outside of the general config
// permit loading correct preset/slot at boot with less overhead
void saveCurrentPreset(int preset){
  int baseAddress = 5;
  EEPROM.put(baseAddress, preset);
}
void loadLastPreset() {
  int baseAddress = 5;
  EEPROM.get(baseAddress, selected_preset);
}

void saveToEEPROM(int slot) {
  int baseAddress = EEPROM_START_ADDRESS + (slot * CONFIG_SIZE);
  if (baseAddress + CONFIG_SIZE <= EEPROM.length()) {
    currentConfig.tempo = tempo;
    currentConfig.kit = kit;
    currentConfig.internalClock = internalClock;
    currentConfig.lastUsedSlot = slot;
    currentConfig.selectedPreset = selected_preset;
    EEPROM.put(baseAddress, currentConfig);
    // Save the last used slot
    EEPROM.put(LAST_USED_SLOT_ADDRESS, slot);
  } else {
    // Handle error
    if (debug ) Serial.println("EEPROM Save Error");
    return;
  }
}
void loadFromEEPROM(int slot) {
  int baseAddress = EEPROM_START_ADDRESS + (slot * CONFIG_SIZE);
  if (baseAddress + CONFIG_SIZE <= EEPROM.length()) {
    EEPROM.get(baseAddress, currentConfig);
    tempo = currentConfig.tempo;
    kit = currentConfig.kit;
    internalClock = currentConfig.internalClock;
    lastUsedSlot = slot;
    selected_preset = currentConfig.selectedPreset;
    //period = 60000 / tempo / 4;  // Update period with loaded tempo
    setup_complete = true;
    updateRythm();               // Refresh
    mode = 0; // return to drum interface
  } else {
    // Handle the error
    if (debug ) Serial.println("EEPROM Save Error");
    return;
  }
}

void initializeCurrentConfig(bool loadDefaults = false) {
  if (loadDefaults) {
    // Load default configuration from PROGMEM
    memcpy_P(&currentConfig, &defaultSlots[0], sizeof(ConfigSlot));  // Load the first default slot as the initial configuration
  } else {
    // Load configuration from EEPROM
    int baseAddress = EEPROM_START_ADDRESS;  // Start address for the first slot
    EEPROM.get(baseAddress, currentConfig);
    tempo = currentConfig.tempo;                     // Load tempo
    internalClock = currentConfig.internalClock;     // Load clock state
    lastUsedSlot = currentConfig.lastUsedSlot;       // Load last used slot
    selected_preset = currentConfig.selectedPreset;  // Load last used preset
    if (selected_preset < NUM_PRESETS) {             // we use overflow to determine if we're in eeprom
      loadFromPreset(selected_preset);
    } else {
      loadFromEEPROM(lastUsedSlot);
    }
  }
}
