#include "booster_mngr.h"
#include "dcc.h"
#include "pwm.h"
#include "off.h"
#include "error.h"

struct booster_mngr_struct *booster_mngr[] = {
  &offBoosterMngr,
  &pwmBoosterMngr,
  &dccBoosterMngr,
  NULL,
};
int booster_mngr_selected = 0;

#define BOOSTER_MNGR_N ((sizeof(booster_mngr)) / (sizeof(struct booster_mngr_struct *)))

void boosterMngrSelect(int mngr)
{
	booster_mngr[booster_mngr_selected]->fini();
	booster_mngr_selected = mngr;
	booster_mngr[booster_mngr_selected]->init();
}

void boosterMngrSelectByName(char *mngr)
{
	for(int i = 0; i < BOOSTER_MNGR_N; i++)
		if(strcasecmp(booster_mngr[i]->name, mngr)) {
			boosterMngrSelect(i);
			return;
		}
	fatal("Invalid booster selected.");
}

