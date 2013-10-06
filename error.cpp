///////////////////////////////////////////////////////////////////////////////
// ERROR HANDLING
///////////////////////////////////////////////////////////////////////////////
#include <Arduino.h>
#include "booster.h"
#include "error.h"

void fatal(char *msg)
{
  boosterEmergencyStop();
  Serial.print("FATAL ERROR");
  if(msg) {
    Serial.print(": ");
    Serial.println(msg);
  }
  while(1);
}

void debug(char *format, ...)
{
}

