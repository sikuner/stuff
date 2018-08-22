
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "widget.h"

#ifndef MAX_LINE
#define MAX_LINE 1024
#endif

static void print_usage()
{
	fprintf(stdout, "\nUsage: \n");
	fprintf(stdout, "    gui_cli [-s string] [-b bitmap] \n\n");
}

int main(int argc, char *argv[])
{
	int rc = -1;
	int opt = -1;
	int i = 0;
	
	char string[MAX_LINE] = { 0 };
	char array[4][MAX_LINE] = { {0} };
	char *begin = NULL;	
	char *end = NULL;
	char bitmap[MAX_LINE] = { 0 };
	
	char type = '\0';
	
	if(1 == argc)
	{
		print_usage();
		return 0;
	}
	
	while ((opt = getopt(argc, argv, "s:b:")) != -1)
	{
		switch(opt)
		{
			case 's':
				strncpy(string, optarg, MAX_LINE);
				type = 's';
				break;
			case 'b':
				strncpy(bitmap, optarg, MAX_LINE);
				type = 'b';
				break;
			default:
				print_usage();
				return 0;
		}
	}
	
	fprintf(stdout, "string: %s \n", string);
	fprintf(stdout, "bitmap: %s \n", bitmap);
	
	rc = WidgetInit();
	if (rc)
	{
		fprintf(stderr, "WidgetInit error! \n");
		return -1;
	}
	
	begin = string;
	for(i = 0; i < 4; i++)
	{
		end = strstr(begin, "\\n");
		if(!end)
		{
			strcpy(array[i], begin);
			break;
		}
		
		strncpy(array[i], begin, end-begin);
		begin = end + 2;
	}
	
//	ShowLine0123(array[0], array[1], array[2], array[3]);
	Toast0123(array[0], array[1], array[2], array[3], 0);
	
	return 0;
}

