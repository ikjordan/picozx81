#ifndef HID_USB
#define HID_USB
#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif
void hidInitialise(byte* keyboard);
bool hidReadUsbKeyboard(uint8_t* special, bool usedouble);
int16_t hidKeyboardToJoystick(void);

int16_t hidReadUsbJoystick(int instance);
void hidJoystickToKeyboard(int instance, byte up, byte down, byte left, byte right, byte button);

bool hidNavigateMenu(uint8_t* key);

#ifdef __cplusplus
}
#endif

#endif
