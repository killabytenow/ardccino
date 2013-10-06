#ifndef __HWGUI_H__
#define __HWGUI_H__

// GENERIC UI SCREEN
struct ui_screen {
  struct ui_screen *(*on_event)(struct ui_screen *ui, int event);
  int focus;
};

#define UI_EVENT_IDLE           0
#define UI_EVENT_OPEN           1
#define UI_EVENT_CLOSE          2
#define UI_EVENT_LEFT           11
#define UI_EVENT_RIGHT          12
#define UI_EVENT_UP             13
#define UI_EVENT_DOWN           14
#define UI_EVENT_PRESSED_LEFT   15
#define UI_EVENT_PRESSED_RIGHT  16
#define UI_EVENT_PRESSED_UP     17
#define UI_EVENT_PRESSED_DOWN   18
#define UI_EVENT_SELECT         20

extern struct ui_screen *ui_curr;

struct ui_screen *ui_config_global_evh(struct ui_config_global_struct *ui, int event);
struct ui_screen *ui_hello_evh(struct ui_hello_struct *ui, int event);
struct ui_screen *ui_pwm_evh(struct ui_pwm_struct *ui, int event);

#endif
