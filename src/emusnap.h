#ifndef EMUSNAP_H
#define EMUSNAP_H

// Supported snapshot versions
#define SUPPORTED_VERSION_1 0x00010001          // Major and minor versions
#define SUPPORTED_VERSION_2 0x00020001          // Major and minor versions

#ifdef __cplusplus
extern "C" {
#endif

extern int emu_FileReadBytes(void* buf, unsigned int size);
extern int emu_FileWriteBytes(const void* buf, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif