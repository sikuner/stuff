
#ifndef __TYPES_H__
#define __TYPES_H__

#define MAX_LINE			1024

#define MAX_URL				2048

typedef unsigned int        DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;

#define min(x,y) ({ typeof(x) _x = (x); typeof(y) _y = (y); (void) (&_x == &_y); _x < _y ? _x : _y; })
#define max(x,y) ({ typeof(x) _x = (x); typeof(y) _y = (y); (void) (&_x == &_y); _x > _y ? _x : _y; })

typedef struct _tRect {
	int x, y;
	int w, h;
} Rect;

#endif // __TYPES_H__

