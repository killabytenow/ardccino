#ifndef __BOOSTER_MNGR_H__
#define __BOOSTER_MNGR_H__

struct booster_mngr_struct {
  char *name;
  void (*init)(void);
  void (*fini)(void);
  void (*refresh)(void);
};

extern struct booster_mngr_struct *booster_mngr[];

void boosterMngrSelect(int mngr);
void boosterMngrSelectByName(char *mngr);

#endif
