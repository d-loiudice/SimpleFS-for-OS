#pragma once
#include "bitmap.h"

#define BLOCK_SIZE 512
// this is stored in the 1st block of the disk
typedef struct {
  // Numero totali di blocchi nel disco
  int num_blocks;
  int bitmap_blocks;   // how many blocks in the bitmap, numero di blocchi mappati sulla bitmap
  int bitmap_bytes;  // how many bytes are needed to store the bitmap, grandezza in byte della bitmap
  // Per l'inodemap
  int inodemap_blocks;  // Numero di inode nella inodemap
  int inodemap_bytes;   // Numero di byte necessari per memorizzare la inodemap
  
  int dataFree_blocks;     // free blocks of data
  int dataFirst_free_block;// first block index data 

  int inodeFree_blocks;     // free blocks of inode <- (necessario ????)
  int inodeFirst_free_block;// first block index
} DiskHeader; 

typedef struct {
  DiskHeader* header; // mmapped
  char* bitmap_data_values;  // mmapped (data bitmap)
  char* bitmap_inode_values;  //mmaped <- (necessario introdurlo???)  
  int fd; // for us
} DiskDriver;

/**
   The blocks indices seen by the read/write functions 
   have to be calculated after the space occupied by the bitmap
*/

// opens the file (creating it if necessary_
// allocates the necessary space on the disk
// calculates how big the bitmap should be
// if the file was new
// compiles a disk header, and fills in the bitmap of appropriate size
// with all 0 (to denote the free space);
void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks);

// reads the block in position block_num
// returns -1 if the block is free accrding to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num);

// writes a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num);

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, int block_num);

// returns the first free blockin the disk from position (checking the bitmap)
int DiskDriver_getFreeBlock(DiskDriver* disk, int start);

// writes the data (flushing the mmaps)
int DiskDriver_flush(DiskDriver* disk);