#include "simplefs.h"

#define DEBUG 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYTE_TO_BINARY_PATTERN "%c|%c|%c|%c|%c|%c|%c|%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 


void bits_print(char* arr,int dim){
	printf("[ ");
	int i;
	for(i=0;i<dim;i++ ){
		printf("(");
  		printf(BYTE_TO_BINARY_PATTERN,BYTE_TO_BINARY(arr[i]) );
  		printf(") ");
	}
	printf("] \n");

}

int main(int agc, char** argv) {
	//#if DEBUG
	// prova di linking TODO da rimuovere 
	printf("INIZIO DEBUGGING FUNZIONI...\n");

	
	
	//-------------------BITMAP TESTING-----------------------------------
	
	printf("DEBUGGING BITMAP...\n");


	// converts a bit to a linear index
	int r1=BitMap_indexToBlock(2, 7);
	printf("%d \n ",r1);
	r1=BitMap_indexToBlock(2, 8);
	printf("%d \n ",r1);



	BitMap bm =BitMap_init();	//inizializza la bitmap, creando array (vuoto)
	bits_print(bm.entries,10);
	BitMap* bmap = malloc(sizeof(BitMap));
	memcpy(bmap,&bm,sizeof(BitMap));

	bits_print( bmap->entries,10);

	//char* cc=calloc(sizeof(char),100);
	//strcpy(cc,"");
	//bits_print(cc,5);
	// sets the bit at index pos in bmap to status
	r1= BitMap_set(bmap, 270, 1);

	printf("set %d \n ",r1);

	r1= BitMap_set(bmap, 20, 1);

	printf("set %d \n ",r1);

	bits_print(bmap->entries,10);

	r1=BitMap_get( bmap, 270, 1);

		printf("get %d \n ",r1);

	r1=BitMap_get( bmap, 2, 1);

		printf("get %d \n ",r1);

	BitMapEntryKey k = BitMap_blockToIndex(3);
	printf("%d \n %d \n",k.entry_num,k.bit_num);


	//-------------------DISK_DRIVER TESTING-----------------------------------

	printf("----| DEBUGGING DISK_DRIVER |----: \n");
	printf("\t init: void\n"); DiskDriver_init(NULL,"fileTest.txt",2);
	/*
	printf("\t readBlock: void\n"); DiskDriver_readBlock(disk, void* dest, int block_num);
	printf("\t writeBlock: void\n");DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num);
	printf("\t freeBlock: void\n"); DiskDriver_freeBlock(DiskDriver* disk, int block_num);
	printf("\t getFreeBlock: void\n"); DiskDriver_getFreeBlock(DiskDriver* disk, int start);
	printf("\t flush: void\n"); DiskDriver_flush(DiskDriver* disk);
	*/

	//-------------------SIMPLE_FS TESTING-----------------------------------

	printf("----| DEBUGGING simplefs |----: \n");
	printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
	printf("DataBlock size %ld\n", sizeof(FileBlock));
	printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
	printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));


	

 // #endif
}
