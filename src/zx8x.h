#ifndef _ZX8X_H_
#define _ZX8X_H_

#ifdef __cplusplus
extern "C" {
#endif

extern void z8x_Init(void);
extern bool z8x_Step(void);
extern void z8x_Start(const char * filename);
extern void z8x_updateValues(void);
extern char* z8x_getFilenameDirectory(void);

extern bool save_snap_zx8x(void);
extern bool load_snap_zx8x(void);

#ifdef __cplusplus
}
#endif

#endif
