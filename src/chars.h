#ifndef _CHARS_H_
#define _CHARS_H_
#ifdef __cplusplus
extern "C" {
#endif

void charSetScreenPtr(uint8_t* screen);
void charSetChromaPtr(uint8_t* screen);
void charSetScreenFont(bool zx80);
void charSetConvert(bool zx80);
void charXorRow(uint32_t row);
void charWriteString(const char* s, uint32_t col, uint32_t row);
void charWriteInvertString(const char* s, uint32_t col, uint32_t row, bool invert);
void charWriteChar(char c, uint32_t col, uint32_t row);
void charInvertChar(char c, uint32_t col, uint32_t row);

#ifdef __cplusplus
}
#endif

#endif