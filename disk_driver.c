#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h> 
#include <fcntl.h>


#include "disk_driver.h"

#define ERROR -1


static DiskDriver* createMMAP(size_t size, int mfd){
	void * addr= 0;
	int protections=PROT_READ | PROT_WRITE;
	int flags = MAP_SHARED | MAP_ANONYMOUS;

	off_t offset=0;

	DiskDriver* state = mmap(addr,size,protections,
		flags, mfd, offset);

	if(state==MAP_FAILED){
		perror("errore in mmap");
		exit(1);
	}


	return state;

}

static void deleteMMAP(void* addr){

	if (ERROR == munmap(addr,sizeof(DiskDriver) )){
		perror("errore in unmap di deleteMMAP");
		exit(1);
	}
}


// opens the file (creating it if necessary_
// allocates the necessary space on the disk
// calculates how big the bitmap should be

// if the file was new
// compiles a disk header, and fills in the bitmap of appropriate size
// with all 0 (to denote the free space);
void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks){

	char file_existance;
	if( access( filename, F_OK ) != -1 ) {
    // file exists
		file_existance=1;
	} else {
    // file doesn't exist
		file_existance=0;
	}

	//TODO possibile evitare controllo interno su esistenza file in fopen
	int fd=open(filename, O_RDWR | O_CREAT);
	if(fd==-1){
		perror("errore in apertura file");
		exit(1);
	}
	if(!file_existance){}	//if the file was new...
	DiskHeader* dh=(DiskHeader*) malloc(sizeof(DiskHeader));
	
	dh->num_blocks=num_blocks;
		//TODO
		dh->bitmap_blocks= -1;  // how many blocks in the bitmap
	dh->bitmap_entries= -1;  // how many bytes are needed to store the bitmap

	dh->free_blocks=-1;     // free blocks
	dh->first_free_block=-1;	// first block index
	


	DiskDriver* dd=(DiskDriver*) malloc(sizeof(DiskDriver));
	dd->header=dh;	// mmapped


	DiskDriver* state = createMMAP(sizeof(DiskDriver),fd);



	// void* pmap = mmap(0, mystat.st_size, PROT_READ | PORT_WRITE,
	// 			MAP_SHARED, fd, 0); 

	// if( pmap == MAP_FAILED)
	// {
	// 	perror("errore in mmap");
	// 	close(fd);
	// 	exit(1);
	// }



	//TODO FILLARE LA bitmap e altro


	if( close(fd)){
		perror("errore in fclose");
		exit(1);
	}

	deleteMMAP(state);
	*disk=*dd;	//side effect e mi do 
	free(dd);

}

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

