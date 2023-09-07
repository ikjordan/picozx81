#ifndef _MENU_H_
#define _MENU_H_

#ifdef __cplusplus
extern "C" {
#endif

extern bool loadMenu(void);
extern bool statusMenu(void);
extern void pauseMenu(void);
extern bool modifyMenu(void);
extern bool restartMenu(void);
extern void rebootMenu(void);
extern bool saveMenu(uint8_t* save, uint length);
#ifdef __cplusplus
}
#endif

#endif
