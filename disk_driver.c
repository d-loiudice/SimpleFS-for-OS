#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include "disk_driver.h"

const int BLOCK_SIZE=512;
int MAX_BLOCK;

typedef struct {
  int num_blocks;
  int bitmap_blocks;   // how many blocks in the bitmap
  int bitmap_entries;  // how many bytes are needed to store the bitmap
  
  int free_blocks;     // free blocks
  int first_free_block;// first block index
} DiskHeader; 

typedef struct {
  DiskHeader* header; // mmapped
  char* bitmap_data;  // mmapped (bitmap)
  int fd; // for us
} DiskDriver;

void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks) {
	MAX_BLOCK=disk->header->num_blocks; 
	fd= fopen(filename, "w+b");//writing + binary, ovvero non è trattato come testo
	if (fd==NULL) {
		printf("impossibile creare disk_driver");
		return -1;	
	}
	int i;
	int j;
	for (i=0; i<MAX_BLOCK;i++) {
		for (j=0; j<BLOCK_SIZE; j++) {
			fputc(0, fd);
		}

	}
	return 0;
}