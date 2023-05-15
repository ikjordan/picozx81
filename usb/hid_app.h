#ifndef HID_APP_H
#define HIP_APP_H
#include "tusb.h"

#define MASK_JOY_RIGHT 0x0001
#define MASK_JOY_LEFT  0x0002
#define MASK_JOY_UP    0x0004
#define MASK_JOY_DOWN  0x0008
#define MASK_JOY_BTN   0x0010

typedef struct {
  int ud;
  int lr;
  int button;
} joystick_state_t;

#ifdef __cplusplus
extern "C" {
#endif
void hid_app_get_latest_keyboard_report(hid_keyboard_report_t* latest);
bool hid_app_get_latest_joystick_state(joystick_state_t* latest, int num);

// Test interface
void hid_app_print_keys(void);
#ifdef __cplusplus
}
#endif

#endif