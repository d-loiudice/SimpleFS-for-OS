#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
/*	BIT MATH OPERATIONS MACRO */
//get the bit in i position of c
#define bit_get(c,i) ((c) & (1<<(7-i) ))	
//set the bit in i position of clear  
#define bit_set(c,i) ((c) |= (1<<(7-i) ))	
//clear the bit in i position of c 
#define bit_clear(c,i) ((c) &= ~(1<<(7-i) ))	
//flip the bit in i position of c
#define bit_flip(c,i) ((c) ^= (1<<(7-i) ))	
//check if the bit in i of c is one (1) or zero (0)
#define bit_is_one(c,i) (( (c) & (1<<(7-i) ) )> 0)


typedef struct{
  int num_bits;
  char* entries;
}  BitMap;

typedef struct {
  int entry_num;
  char bit_num;
} BitMapEntryKey;

// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
BitMapEntryKey BitMap_blockToIndex(int num);

// converts a bit to a linear index
int BitMap_indexToBlock(int entry, uint8_t bit_num);

// returns the index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
int BitMap_get(BitMap* bmap, int start, int status);

// sets the bit at index pos in bmap to status
int BitMap_set(BitMap* bmap, int pos, int status);

/*
// INODES

// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
BitMapEntryKey BitMapInode_blockToIndex(int num);

// converts a bit to a linear index
int BitMapInode_indexToBlock(int entry, uint8_t bit_num);

// returns the index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
int BitMapInode_get(BitMap* bmap, int start, int status);

// sets the bit at index pos in bmap to status
int BitMapInode_set(BitMap* bmap, int pos, int status);
*/
BitMap BitMap_init(void);	//inizializza la bitmap, creando array (vuoto)
