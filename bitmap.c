
#include "bitmap.h"

//Data BitMap: char array containing in each bit a 0 if the data block is free 1 instead
	//ex:[ 0111 0110 | 0110 0001]	blocco 0 free, blocco 1 occupato ecc...




#define ENTRIES_DEFAULT_NUM 256



// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
BitMapEntryKey BitMap_blockToIndex(int num){

	BitMapEntryKey b_key;

	
	b_key.entry_num = num/8; 
	b_key.bit_num = num % 8;
	return b_key;	//TODO <- non buona idea allocazione dinamica??? poi la free???
}


// converts a bit to a linear index
int BitMap_indexToBlock(int entry, uint8_t bit_num){ 
	if(bit_num<0 || bit_num >7 || entry < 0 ){
		perror("invalid values");
		return -1;
	}
	return (entry*8)+bit_num;   
}

// returns the LINEAR index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
//so status is 1 or 0 

//search from start block(!) to the end 
int BitMap_get(BitMap* bmap, int start, int status){
	int i;
	int j;

	BitMapEntryKey k=BitMap_blockToIndex(start);
	if(start<0 || start >= ENTRIES_DEFAULT_NUM){
		perror("invalid start in bitmap");
		return -1;
	}
	for(i=k.entry_num; i < bmap->num_bits/8;i++){	//start from start block
		for (j=k.bit_num ;j<8;j++){
			//printf("%d ",bit_get(bmap->entries[i],j));
			char r=bit_get(bmap->entries[i],j) !=0 ? 1 : 0;
			if(  r== status)
				return BitMap_indexToBlock(i,j);	//LINEAR
		}
		j=0;
	}
	return -1;
}

// sets the bit at index pos in bmap to status
int BitMap_set(BitMap* bmap, int pos, int status){
	fprintf(stderr, "BitMap_set() -> setto posizione %d a %d\n", pos, status);


	int i;
	int j;
	BitMapEntryKey k=BitMap_blockToIndex(pos);
	
	if(pos<0 || pos >= ENTRIES_DEFAULT_NUM){
		perror("invalid start in bitmap");
		return -1;
	}

	if(status==1)
		bit_set(bmap->entries[k.entry_num],k.bit_num);
	else
		bit_clear(bmap->entries[k.entry_num],k.bit_num);

	return status;
}

BitMap BitMap_init(void){

	BitMap bmap;
	bmap.num_bits=ENTRIES_DEFAULT_NUM * 8;
	char* arr= (char*)malloc(sizeof(char)*ENTRIES_DEFAULT_NUM);		//<- memory leakage
	strncpy(arr,"",ENTRIES_DEFAULT_NUM);
	bmap.entries=arr;
	return bmap;
}

