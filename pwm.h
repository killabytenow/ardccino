#ifndef __PWM_H__
#define __PWM_H__

#include "booster_mngr.h"

#define PWM_OUTPUT_MODE_DIRECT    0
#define PWM_OUTPUT_MODE_INERTIAL  1

extern struct booster_mngr_struct pwmBoosterMngr;
void pwmAccelerate(int ps, int v);
void pwmSpeed(int ps, int v);
void pwmMode(int booster, int mode);

#endif
