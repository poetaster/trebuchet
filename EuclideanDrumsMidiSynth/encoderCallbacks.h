/**
   handle encoder button long press event
*/
void onEb1LongPress(EncoderButton& eb) {

  if (debug) {
    Serial.print("button1 longPressCount: ");
    Serial.println(eb.longPressCount());
  }
}
/**
   handle encoder turn with  button pressed
*/
void onEb1PressTurn(EncoderButton& eb) {
  long enc_delta = eb.increment();

  if (debug) {
    Serial.print("eb1 press inc by: ");
    Serial.println(enc_delta);
    Serial.print("enc_offset is: ");
    Serial.println(enc_delta);
  }
}

/**
   handle encoder turn with  button pressed
*/
void onEb1Clicked(EncoderButton& eb) {

  // set which bank to select formulas from
  //bank = eb.clickCount();

  if (debug) {
    Serial.print("bank: ");
    Serial.println(eb.clickCount());
  }

}



/**
   A function to handle the 'encoder' event without button
*/
void onEb1Encoder(EncoderButton& eb) {
  encoder_delta = eb.increment();
    if (mode == 0) {
      kit = constrain( (kit + encoder_delta), 0, 8);
      if (debug) {
        Serial.print("kit: ");
        Serial.println(kit);
      }
      uiUpdate = true;
  } else {
      instrument = constrain( ( instrument + encoder_delta), 0, 128);
      vsmidi.sendMIDI(0xC0 | 0, instrument, 0);
      synthDisplayUpdate();
  }
  if (debug) {
    Serial.print("eb1 incremented by: ");
    Serial.println(encoder_delta);
  }
}
