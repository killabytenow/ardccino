#include <memorysaver.h>

#include "booster.h"
#include "dcc.h"

void setup()
{
  int i;

  delay(10000);
  
  //Serial.begin(9600);
  Serial.begin(115200);
  Serial.println("Initializing");
  
  boosterSetup();
  joySetup();
  utftSetup();
  
  // init menu ui
  //ui_curr = (struct ui_screen *) &ui_hello;
  //current_ui_handler = uiHandler;
  current_ui_handler = cliHandler;
}


void loop()
{
  booster_mngr[booster_mngr_selected].refresh();
  current_ui_handler();
  
  delay(100);
}

