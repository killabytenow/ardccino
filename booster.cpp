///////////////////////////////////////////////////////////////////////////////
// BOOSTER CONFIG
///////////////////////////////////////////////////////////////////////////////

#include "Arduino.h"
#include "booster.h"
#include "booster_mngr.h"

//   - Pins 5 and 6 are paired on timer0
//   - Pins 9 and 10 are paired on timer1
//   - Pins 3 and 11 are paired on timer2
struct booster_struct booster[] = {
    { "booster#1",  3,  2,   4,  5, 14 },
    { "booster#2",  9,  6,   7,  8, 14 },
    { "booster#3", 10, 11,  12, 13, 14 },
//  { "booster#4", xx, xx,  xx, xx, xx },
};

int booster_mngr_selected = 0;

#define BOOSTER_N      ((sizeof(booster)) / (sizeof(struct booster_struct)))
#define BOOSTER_MNGR_N ((sizeof(booster_mngr)) / (sizeof(struct booster_mngr_struct)))

void boosterSetup(void)
{
  Serial.print("Declared ");
  Serial.print(BOOSTER_N);
  Serial.println(" boosters.");
  for(int i = 0; i < BOOSTER_N; i++) {
    Serial.print("  - Configuring booster #");
    Serial.print(i);
    Serial.print(" (");
    Serial.print(booster[i].name);
    Serial.println(")");

    pinMode(booster[i].pwmSignalPin, OUTPUT); digitalWrite(booster[i].pwmSignalPin, LOW);
    pinMode(booster[i].dirSignalPin, OUTPUT); digitalWrite(booster[i].dirSignalPin, LOW);
    pinMode(booster[i].rstSignalPin, OUTPUT); digitalWrite(booster[i].rstSignalPin, LOW);
    pinMode(booster[i].tmpAlarmPin, INPUT);
    pinMode(booster[i].ocpAlarmPin, INPUT);
  }
}

void boosterEmergencyStop(void)
{
  Serial.println("Booster emergency stop!");
  
  // finish current booster manager and select off
  boosterMngrSelect(0);
}


