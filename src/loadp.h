#ifndef _LOADP_H_
#define _LOADP_H_

bool loadPInitialise(char* fullpath, int filename, bool ROM4k);
void loadPUninitialise(void);
int loadPGetBit(void);

#endif // _LOADP_H_
