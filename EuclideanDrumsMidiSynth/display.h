
bool uiUpdate = false;
bool haveDisplay = false;

// util int to char array
void int_to_char(int num, char *result) {
  int temp = num;
  int len = 0;

  while (temp > 0) {
    len++;
    temp /= 10;
  }

  for (int i = len - 1; i >= 0; i--) {
    result[i] = num % 10 + '0';
    num /= 10;
  }

  result[len] = '\0';
}

// general display update
void drumsDisplayUpdate() {
  int i, x, y;
  oledFill(&ssoled, 0, 1);

  char result[100];
  int_to_char(tempo, result);
  oledWriteString(&ssoled, 0, 0, 0, (char *)"BPM ", FONT_STRETCHED, 0, 1);
  oledWriteString(&ssoled, 0, 58, 0, result, FONT_STRETCHED, 0, 1);

  char kits[100];
  int_to_char(kit + 1, kits);
  oledWriteString(&ssoled, 0, 0, 3, (char *)"K", FONT_STRETCHED, 0, 1);
  oledWriteString(&ssoled, 0, 24, 3, kits, FONT_STRETCHED, 0, 1);

  char fills[100];
  int_to_char(numberOfFills, fills);
  oledWriteString(&ssoled, 0, 56, 3, (char *)"FLS ", FONT_STRETCHED, 0, 1);
  oledWriteString(&ssoled, 0, 96, 3, fills, FONT_STRETCHED, 0, 1);

  char reps[100];
  int_to_char(repeats, reps);
  oledWriteString(&ssoled, 0, 0, 6, (char *)"REP", FONT_STRETCHED, 0, 1);
  oledWriteString(&ssoled, 0, 72, 6, reps, FONT_STRETCHED, 0, 1);

}
// general display update
void synthDisplayUpdate() {
  int i, x, y;
  oledFill(&ssoled, 0, 1);

  int offset = 0;
  char * pch;
  //printf ("Splitting string \"%s\" into tokens:\n",str);
  pch = strtok (buffer," -");
  while (pch != NULL)
  {
    oledWriteString(&ssoled, 0, 0, offset, (char *)pch, FONT_STRETCHED, 0, 1);
    //printf ("%s\n",pch);
    offset = offset + 3;
    pch = strtok (NULL, " ,.-");
  }
}

// setup function, oled
void displaySetup() {
  int rc;
  //rc = oledInit(&ssoled, OLED_128x64, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, SDA_PIN, SCL_PIN, -1, 400000L);
  rc = oledInit(&ssoled, OLED_128x64, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, -1, -1, -1, 400000L);
  // Standard HW I2C bus at 400Khz

  if (rc != OLED_NOT_FOUND)
  {
    char *msgs[] =
    {
      (char *)"SSD1306 @ 0x3C",
      (char *)"SSD1306 @ 0x3D",
      (char *)"SH1106 @ 0x3C",
      (char *)"SH1106 @ 0x3D"
    };

    oledFill(&ssoled, 0, 1);
    oledWriteString(&ssoled, 0, 16, 2, (char *)"treBu", FONT_STRETCHED, 0, 1);
    oledWriteString(&ssoled, 0, 24, 4, (char *)"cHet", FONT_STRETCHED, 0, 1);
    //oledWriteString(&ssoled, 0, 10, 4, msgs[rc], FONT_NORMAL, 0, 1);

    haveDisplay = true;
    delay(1000);
  }
}
