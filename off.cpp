#include "booster.h"
#include "booster_mngr.h"
#include "off.h"

struct booster_mngr_struct offBoosterMngr =
	{ "off", offStart, offStop, offRefresh };

void offStart(void)
{
	for(int b = 0; b < BOOSTER_N; b++) {
		boosterPwmSet(b, LOW);
		boosterDirSet(b, LOW);
	  }
}

void offStop(void)
{
}

void offRefresh(void)
{
}

