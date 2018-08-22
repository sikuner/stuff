
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

void dbg_printf(const char *file, int line, const char *func, const char *format, ...)
{
	char szLog[1024] = { 0 };
	va_list ap;
	va_start(ap, format);
	vsprintf(szLog, format, ap);
	va_end(ap);
	
	fprintf(stderr, "%s:%d %s(): %s", file, line, func, szLog);
}

/* naive function to check whether char *s is an ip address */
int is_ip_address(const char *s)
{
	while (*s) 
	{
		if ((isdigit(*s)) || (*s == '.')) 
		{
			s++;
			continue;
		}
		
		return 0;
	}
	
	return 1;
}

