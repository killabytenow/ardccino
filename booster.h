#ifndef __BOOSTER_H__
#define __BOOSTER_H__

#define BOOSTER_STATUS_OK     0
#define BOOSTER_STATUS_OVP    1
#define BOOSTER_STATUS_HEAT   2

#define BOOSTER_N      3
#define BOOSTER_MNGR_N 3

struct booster_struct {
  char          *name;
  unsigned char  pwmSignalPin;
  unsigned char  dirSignalPin;
  unsigned char  tmpAlarmPin;  // temperature alarm
  unsigned char  ocpAlarmPin;  // over current protection alarm
  unsigned char  rstSignalPin; // reset pin
};

extern struct booster_struct booster[];

#define boosterPwmSet(b,v)                 \
	digitalWrite(                      \
		booster[(b)].pwmSignalPin, \
		(v))

#define boosterDirSet(b,v)                 \
	digitalWrite(                      \
		booster[(b)].dirSignalPin, \
		(v))

void boosterSetup(void);
void boosterEmergencyStop(void);

#endif
