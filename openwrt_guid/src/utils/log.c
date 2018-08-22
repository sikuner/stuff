
#include "log.h"

#include <stdio.h>
#include <stdarg.h>

static int debug_level_limit = 8; // ±»¥Û–°

void dbg_printf(const char *file, int line, const char *func, const char *format, ...)
{
	char szLog[1024] = { 0 };
	va_list ap;
	va_start(ap, format);
	vsprintf(szLog, format, ap);
	va_end(ap);
	
	fprintf(stderr, "%s:%d %s(): %s\n", file, line, func, szLog);
}

