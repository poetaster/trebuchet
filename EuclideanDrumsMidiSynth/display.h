
bool uiUpdate = false;
bool haveDisplay = false;

void ReplaceCharactersInString(char *pcString, char cOldChar,char cNewChar){
    while ( pcString && *pcString) {//not NULL and not zero
        if ( *pcString == cOldChar) {//match
            *pcString = cNewChar;//replace
        }
        ++pcString;//advance to next character
    }
}

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

  char result[16];
  int_to_char(tempo, result);
  oledWriteString(&ssoled, 0, 0, 0, (char *)"T ", FONT_STRETCHED, 0, 1);
  oledWriteString(&ssoled, 0, 24, 0, result, FONT_STRETCHED, 0, 1);

  char kits[16];
  int_to_char(kit + 1, kits);
  oledWriteString(&ssoled, 0, 84, 0, (char *)"K", FONT_STRETCHED, 0, 1);
  oledWriteString(&ssoled, 0, 100, 0, kits, FONT_STRETCHED, 0, 1);

  char fills[16];
  int_to_char(seq[current_track].fills, fills);
  oledWriteString(&ssoled, 0, 0, 3, (char *)"F ", FONT_STRETCHED, 0, 1);
  oledWriteString(&ssoled, 0, 24, 3, fills, FONT_STRETCHED, 0, 1);

  char reps[16];
  int_to_char(seq[current_track].repeats, reps);
  oledWriteString(&ssoled, 0, 64, 3, (char *)"R ", FONT_STRETCHED, 0, 1);
  oledWriteString(&ssoled, 0, 96, 3, reps, FONT_STRETCHED, 0, 1);

  char text[16];
  int_to_char(current_track+1, text);
  //ReplaceCharactersInString ( text, '0', '.');
  oledWriteString(&ssoled, 0, 0, 6, seq[current_track].trigger->textSequence, FONT_SMALL, 0, 1);
  oledWriteString(&ssoled, 0, 112, 6, text, FONT_STRETCHED, 0, 1);
  

}

void displayPattern() {
  // textSequence is the var which holds the euclidean
  //char kits[32];
  //int_to_char(text, kits);
  oledFill(&ssoled, 0, 1);  
  ReplaceCharactersInString ( textSequence, '0', '.');
  oledWriteString(&ssoled, 0, 0, 4, textSequence, FONT_SMALL, 0, 1);
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
