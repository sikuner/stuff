
#ifndef __OLED_API_H__
#define __OLED_API_H__

int  OledDrvInit();
void OledDrvRelease();

void ShowPattern(unsigned char *Data_Pointer, unsigned char a, unsigned char b, unsigned char c, unsigned char d);

#endif // __OLED_API_H__

