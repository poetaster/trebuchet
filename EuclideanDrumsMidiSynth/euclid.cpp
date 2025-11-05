#include <Arduino.h>
#include "euclid.h"

void euclid::generateSequence( uint8_t fills, uint8_t steps) {
  numberOfSteps = steps;
  if (fills > steps) fills = steps;
  if (fills <= steps) {
    for (int i = 0; i < 16; i++) euclidianPattern[i] = false;
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
  //Serial.println(this->getRepeats());
  if (this->getRepeats()> 0) {
    
    euclid::rotate(repeats);
  }
  // now generate the text rep.
  euclid::setTextSequence();
}

void euclid::generateRandomSequence( uint8_t fills, uint8_t steps) {
  //if(numberOfSteps!=steps && numberOfFills!=fills){
  numberOfSteps = steps;
  numberOfFills = fills;
  if (fills > steps) fills = steps;
  if (fills <= steps) {
    for (int i = 0; i < 16; i++) euclidianPattern[i] = false;
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
            euclidianPattern[where] = true; //,Serial.print(where),Serial.print(" ");
            break;
          }

        }
        //}
      }
      //Serial.println();
    }
  }
  //}
  // now generate the text rep.
  euclid::setTextSequence();
}

void euclid::rotate(uint8_t _steps) {

  for (int i = 0; i < _steps; i++) {

    bool temp = this->euclidianPattern[numberOfSteps - 1];

    for (int j = numberOfSteps; j > 0; j--) {
      euclidianPattern[j] = euclidianPattern[j - 1];
    }
    euclidianPattern[0] = temp;
  }
  euclid::setTextSequence();

}

bool euclid::getStep(uint8_t _step) {
  return euclidianPattern[_step];
}
bool euclid::getCurrentStep() {
  return euclidianPattern[stepCounter];
}

uint8_t euclid::getCurrentRepeat() {
  return repeatCounter;
}
void euclid::setRepeats(uint8_t _repeats) {
  this->repeats = this->repeats + _repeats;
  if (repeats < 0) {
    this->repeats = 0;
  }
}

uint8_t euclid::getRepeats() {
  return repeats;
}

/* generate a string rep of the pattern */

void euclid::setTextSequence() {
  for (int i = 0; i < NUMBER_OF_STEPS; i++) textSequence[i] = (euclidianPattern[i] == 0) ? '.' : '1';
}

char euclid::getTextSequence() {
  return *textSequence;
}
void euclid::doStep() {
  if (this->stepCounter < (numberOfSteps - 1)) {
    this->stepCounter++;

  } else {
    ;
    this->stepCounter = 0;
    if (this->repeats > 0) this->repeatCounter++;

    if (this->repeatCounter >= this->repeats) {
      //this->generateSequence(this->numberOfSteps, this->numberOfFills);
      //euclid::rotate(1);
      //Serial.println(this->textSequence);
      this->repeatCounter = 0;
    }
  }
}

void euclid::resetSequence() {
  stepCounter = 0;
}
