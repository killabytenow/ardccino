#ifndef __DCC_H__
#define __DCC_H__

#include <Arduino.h>
#include "booster_mngr.h"

#define DCC_MSG_MAX           5

struct dcc_buffer_struct {
  byte msg[DCC_MSG_MAX];
  unsigned int  address;
//unsigned int command;
  unsigned char len;
  char reps;
};

void dccStart(void);
void dccStop(void);
void dccRefresh(void);
struct dcc_buffer_struct *dccSendBuffer(byte *msg, unsigned char len);

extern struct booster_mngr_struct dccBoosterMngr;

#endif
