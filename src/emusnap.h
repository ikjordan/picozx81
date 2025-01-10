#ifndef EMUSNAP_H
#define EMUSNAP_H

#ifdef __cplusplus
extern "C" {
#endif

extern int emu_FileReadBytes(void* buf, unsigned int size);
extern int emu_FileWriteBytes(const void* buf, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif