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

int kits[KIT][9] = {
  { 35, 36, 35, 38, 40, 38, 46, 28, 42}, // clicks, side stick, snare bass
  { 35, 36, 64, 38, 38, 31, 42, 46, 62 }, // not bad
  { 35, 36, 42, 38, 40, 46, 70, 28, 37 }, // not bad
  { 35, 36, 35, 38, 40, 63, 46, 70, 42 }, // little more on the snare side/ HH, lo & hi conga, and maracas
  { 35, 36, 64, 38, 40, 38, 28, 37, 63,}, // little more on the snare side/ HH, lo conga, one clave hit
  { 35, 36, 46, 38, 42, 38, 27, 44, 64 }, // clicks snare side stick maracas
  { 35, 64, 64, 38, 39, 39, 46, 42, 62 }, // clicks, hh, castenets
  { 35, 64, 35, 38, 62, 31, 42, 37, 33 },
  { 35, 36, 62, 38, 40, 38, 82, 69, 31 },
}; // Bass, Snare, HHO, HHCs
