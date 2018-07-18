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

int fp=-1;
int PageSize; 

static DiskDriver* createMMAP(void* addr,size_t size, int mfd){
	
	
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

				//LAVORO PER SINGOLO FILE (un file creato-scritto/letto-chiuso)TODO DA AMPLIARE

// opens the file (creating it if necessary_
// allocates the necessary space on the disk
// calculates how big the bitmap should be

// if the file was new
// compiles a disk header, and fills in the bitmap of appropriate size
// with all 0 (to denote the free space);

//side effect su disk
void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks){

	// 2 diversi casi: il file è creato / il file è aperto
	char file_existance=0;
	if( access( filename, F_OK ) != -1 ) {
    // file exists
		file_existance=1;
	} else {
    // file doesn't exist
		file_existance=0;
	}

	PageSize= (int)sysconf(_SC_PAGESIZE);	//serve per la mmap
    if ( PageSize < 0) {
    	perror("sysconf() error");
    }


	//TODO possibile evitare controllo interno su esistenza file in fopen
	fp=open(filename, O_RDWR | O_CREAT|O_APPEND,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(fp==-1){
		perror("errore in apertura file");
		exit(1);
	}

	DiskHeader* dh=(DiskHeader*) malloc(sizeof(DiskHeader));
	// Bisogna usare l'indirizzo passato tramite parametro nella funzione (disk)
	// Bisogna scegliere se effettuare l'allocazione fuori o dentro la funzione
	DiskDriver* dd=(DiskDriver*) malloc(sizeof(DiskDriver));
	DiskDriver* dd_ptr;
	if(!file_existance){//if the file was new...
		//init di diskHeader e diskdriver 
	// Se il file non esiste, inizializzo la struttura DiskHeader per poi scriverla sul file 
	// e inizializzazione della BitMap e dell'InodeMap (inizializzare tutti 0)	

	  BitMap data_bm=  BitMap_init();	//tutte a 0
	  BitMap inode_bm=  BitMap_init();
	  int free_data_counter= data_bm.num_bits;
	  int free_inode_counter= inode_bm.num_bits;

	  //On Header...
	  int i,dpos;
	  for(i=0;i<num_blocks;i++){
		  dpos=BitMap_get(&data_bm,0,0); // search in bitmap first bit for 0
		  if(dpos!=0){
		  	perror("not found place in data bitmap");		  
		  	exit(1);
		  	}
		  BitMap_set(&data_bm,dpos,1);
		  free_data_counter--;
	  }

	  dh->bitmap_blocks= data_bm.num_bits;   // numero di blocchi mappati sulla bitmap, per ora 0
	  dh->bitmap_bytes= data_bm.num_bits/8 +1 ;  //  grandezza in byte della bitmap: ogni blocco è un bit 
	  // Per l'inodemap
	  dh->inodemap_blocks= inode_bm.num_bits ;  // Numero di inode nella inodemap, just one for the file
	  dh->inodemap_bytes= inode_bm.num_bits/8 + 1;   // Numero di byte necessari per memorizzare la inodemap, just one at the moment
	  
	  dh->dataFree_blocks= free_data_counter;     // free blocks of data, dim della bitmap in blocchi - num_blocks ???
	  dh->dataFirst_free_block= BitMap_get(&data_bm,0,0);// first block index data 

	  dh->inodeFree_blocks= free_inode_counter;     // free blocks of inode <- (necessario ????)
	  dh->inodeFirst_free_block= BitMap_get(&inode_bm,0,0);// first block index
		
	  //On Driver...
	  dd->header= dh; 
  	  dd->bitmap_data_values= data_bm.entries;  
  	  dd->bitmap_inode_values= inode_bm.entries;  
  	  dd->fd=fp; // for us

  	  
	}
	else{	//file already existing
			
		// Se il file esiste, leggo il DiskHeader che è memorizzato all'inizio del file
		// leggo di sizeof(DiskHeader)
		// Successivamente riempio la struttura DiskDriver
		// Mmap della BitMap e dell' InodeMap
		//TODO FILLARE LA bitmap perche il file gia è presente e magari contiene roba che va riportata
		
		//TODO very not sure what to do
		dh=disk->header;
		dd=disk;

	}

	dd_ptr = createMMAP(NULL,sizeof(DiskDriver),-1);	// dubbio su mmap di diskheader

	if( close(fp)){
		perror("errore in fclose");
		exit(1);
	}

	disk=dd_ptr;	//side effect e mi do 
	//dd_ptr non freeable

}

// reads the block in position block_num
// returns -1 if the block is free accrding to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num) {
	
	//check if block is free according to bitmap
	char* bm=disk->bitmap_data_values;
	
	BitMapEntryKey k=BitMap_blockToIndex(block_num);

	if( bit_get(bm[k.entry_num],k.bit_num)==0 ){	//intere block (8 celle) all zeros
		perror("cannot read cause block in bitmap is zero");
		return -1;
	}

	void* blockRead=(void*) malloc(BLOCK_SIZE);
	fseek(disk->fd, block_num*BLOCK_SIZE, SEEK_SET);
	fread(blockRead, BLOCK_SIZE, 1, fp);
	int j;
	for (j=0; j<BLOCK_SIZE; j++) {
		memcpy(dest, blockRead, BLOCK_SIZE);
	}
	free(blockRead);
	return 0;
	
	




}

// writes a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num) {
	/*
	void* blockWrite = (void*) malloc(BLOCK_SIZE);
	fseek(fp, block_num*BLOCK_SIZE, SEEK_SET);
	memcpy(blockWrite, src, BLOCK_SIZE);
	fwrite(blockWrite, BLOCK_SIZE, 1, fp);
	fflush(fp);
	free(blockWrite);
	return 0;
	*/
	//TODO sempre il problema della bitmap non ancora implementata

	return -1;


}

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, int block_num);

// returns the first free blockin the disk from position (checking the bitmap)
int DiskDriver_getFreeBlock(DiskDriver* disk, int start);

// writes the data (flushing the mmaps)
int DiskDriver_flush(DiskDriver* disk);

