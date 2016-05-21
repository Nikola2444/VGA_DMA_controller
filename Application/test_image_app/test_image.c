#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lena_hex.h"

int main(void)
{
	FILE* fp;
	int x,y;

	for(y=0; y<480; y++)
	{
		for(x=0; x<640; x++)
		{
			fp = fopen("/dev/vga_dma", "w");
			if(fp == NULL)
			{
				printf("Cannot open /dev/vga for write\n");
				return -1;
			}
			fprintf(fp,"%d,%d,%#04x\n",x,y,image[y*640+x]);
			fclose(fp);
			if(fp == NULL)
			{
				printf("Cannot close /dev/vga\n");
				return -1;
			}
		}
	}
	return 0;
}

