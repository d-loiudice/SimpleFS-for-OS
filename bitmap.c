#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"

// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
BitMapEntryKey BitMap_blockToIndex(int num){

	BitMapEntryKey* b_key=(BitMapEntryKey*)malloc(sizeof(BitMapEntryKey));

	printf("[TODO] not completed");
	b_key->bit_num = num;
	b_key->entry_num = num;

	return *b_key;	//TODO <- non buona idea allocazione dinamica??? poi la free???
}


// converts a bit to a linear index
int BitMap_indexToBlock(int entry, uint8_t bit_num){ //bit_num è un bit 0/1
	return -1;

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
	int i;
	for(i=0;i < bmap->num_bits;i++){
		if(i==pos){
			bmap->entries[i]=status;
			return 0;
		}
	}
	return -1;

}