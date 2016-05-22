#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "../PyGif/giphy.h"
#define MAX_PKT_SIZE (640*480*4)

//comment to send pixels as commands via regular write function of char driver
//leave uncommented to write directly to memory (faster)
#define MMAP

int main(void)
{
	int x,y;
	int i=0;
	#ifdef MMAP
	// If memory map is defined send image directly via mmap
	int fd;
	int *p;
	while(1){
	for(i = 0; i < 12; i++){
		fd = open("/dev/vga_dma", O_RDWR|O_NDELAY);
		if (fd < 0)
		{
			printf("Cannot open /dev/vga for write\n");
			return -1;
		}
		//printf("Sending pic %d\n",i);
		p=(int*)mmap(0,640*480*4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		memcpy(p, giphy[i], MAX_PKT_SIZE);
		munmap(p, MAX_PKT_SIZE);
		close(fd);
		if (fd < 0)
		{
			printf("Cannot close /dev/vga for write\n");
			return -1;
		}
		usleep(50000);
	}}

	#else
	// Send via regualar driver interface
	FILE* fp;
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
			fprintf(fp,"%d,%d,%#04x\n",x,y,giphy0[y*640+x]);
			fclose(fp);
			if(fp == NULL)
			{
				printf("Cannot close /dev/vga\n");
				return -1;
			}
		}
	}

	#endif
	return 0;
}

