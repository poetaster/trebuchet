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
