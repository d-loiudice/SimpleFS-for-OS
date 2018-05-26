#include <stdio.h>
#include <stdlib.h>
#include "data_bitmap.h"
//Data BitMap: char array containing in each bit a 0 if the data block is free 1 instead
	//ex:[ 0111 0110 | 0110 0001]	blocco 0 free, blocco 1 occupato ecc...


// typedef struct{
//   int num_bits;
//   char* entries;
// }  BitMap;

// typedef struct {
//   int entry_num;
//   char bit_num;
// } BitMapEntryKey;

#define ENTRIES_DEFAULT_DIM 256 //256 * 8 sono 2048 bit,cioe blocchi    //TODO spostarla su .h

// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
BitMapEntryKey BitMap_blockToIndex(int num){

	BitMapEntryKey b_key;

	printf("[TODO] not completed");
	b_key.entry_num = num/8; 
	b_key.bit_num = num % 8;
	return b_key;	//TODO <- non buona idea allocazione dinamica??? poi la free???
}


// converts a bit to a linear index
int BitMap_indexToBlock(int entry, uint8_t bit_num){ //bit_num è un bit 0/1 ? visto che è un char...
	return (entry*8)+bit_num;   //TODO not sure
}

// returns the index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
int BitMap_get(BitMap* bmap, int start, int status){
	int i;
	for(i=start;i < bmap->num_bits;i++){
		if(bmap->entries[i]==status)
			return i;
	
	}
	return -1;
}

// sets the bit at index pos in bmap to status
int BitMap_set(BitMap* bmap, int pos, int status){
	
	if(pos <0 || pos >= bmap->num_bits){
		perror("ERR: posizione della bitmap non valida");
		return -1;
	}
	bmap->entries[pos]=status;
	return 0;
}

BitMap BitMap_init(void){

	BitMap bmap;
	bmap.num_bits=ENTRIES_DEFAULT_DIM * 8;
	char* arr=malloc(sizeof(char)*ENTRIES_DEFAULT_DIM);
	bmap.entries=arr;
	free(arr);
	return bmap;
}