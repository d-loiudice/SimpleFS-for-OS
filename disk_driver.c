#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h> 
#include <fcntl.h>
#include <errno.h>

#include "disk_driver.h"
#include "inode.h"

#define ERROR -1
#define NUM_SUPER 1
#define NUM_BITMAPS 2
#define NUM_INODES 5
#define INODES_PER_BLOCK 16


int fp=-1;
int PageSize; 


//LAVORO PER SINGOLO FILE (un file creato-scritto/letto-chiuso)TODO DA AMPLIARE

// opens the file (creating it if necessary_
// allocates the necessary space on the disk
// calculates how big the bitmap should be

// if the file was new
// compiles a disk header, and fills in the bitmap of appropriate size
// with all 0 (to denote the free space);
Inode Inode_init(void){
	Inode inode;
	inode.permessiUtente=0;
	// Permessi del gruppo a cui appartiene il file, valori possibili da 0 a (rwx)
	inode.permessiGruppo=0;
	// Permessi agli altri (rwx) da 0 a 7
	inode.permessiAltri=0;
	// La lista degli utenti si potrebbe registrare su un file nel filesystem ( es. users.pippo )
	// Id dell'utente a cui appartiene il file
	inode.idUtente=0;
	// Id del gruppo 
	inode.idGruppo=0;
	// Data creazione del file
	inode.dataCreazione= time(NULL);
	// Data ultima modifica del file
	inode.dataUltimaModifica = time(NULL);
	// Dimensione del file ( in bytes ) massimo valore 4GB più o meno essendo long int = 32bit
	inode.dimensioneFile=0;
	// Dimensione in blocchi (quanti blocchi occupa il file descritto da questo inode)
	inode.dimensioneInBlocchi=0;
	// Tipo di file ( 'r' = file regolare, 'd' = directory,'u' = unknown yet ecc...,  )
	inode.tipoFile='u';
	// Indice del primo blocco del file (LINKED LIST BLOCKS)
	inode.primoBlocco=0;
	
	return inode;
	
	}


void DiskDriver_init(DiskDriver* diskDriver, const char* filename, int num_blocks){
	int fd;
	DiskHeader* nuovoDiskHeader;
	//DiskHeader* diskHeader;
	//char* bitmapDataValues;
	//char* bitmapInodeValues;
	void* zonaMappata;
	/*int PageSize= (int)sysconf(_SC_PAGESIZE);
	fprintf(stderr, "PageSize: %d\n", PageSize);*/
	
	// Verifica che il file del disco esista
	if ( access(filename, F_OK) == 0 )
	{
		// Il file esiste
		fd = open(filename, O_RDWR);
		if ( fd != -1 )
		{
			// Mmappo i primi 3 blocchi del file, in quanto l'offset è una truffa
			zonaMappata = mmap(NULL, 3*BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			if ( zonaMappata != MAP_FAILED ) 
			{
				diskDriver->header = (DiskHeader*)zonaMappata;
				diskDriver->bitmap_data_values = (char*)(zonaMappata+BLOCK_SIZE);
				diskDriver->bitmap_inode_values = (char*)(zonaMappata+(2*BLOCK_SIZE));
				diskDriver->fd = fd;
				//DiskDriver_flush(diskDriver);
			}
			else
			{
				perror("Errore durante la mmap di tutta la prima parte del file (file esistente)\n");
				exit(1);
			}
		}
		else
		{
			perror("Errore durante l'apertura del file già esistente\n");
			exit(1);
		}
	}
	else
	{
		// Il file non esiste, lo devo creare
		fd = open(filename, O_RDWR | O_CREAT, 0666);
		if ( fd != -1 )
		{
			fprintf(stderr, "FD: %d\n", fd);
			int index = 0;
			while ( index < num_blocks*BLOCK_SIZE )
			{
				// Inizializzo il file tutto con byte 0 
				if ( write(fd, "\0" , 1) <= 0 )
				{
					perror("Errore durante l'inizializzazione del nuovo file\n");
					exit(1);
				}
				index = index + 1;
			}
			// Inizializzazione nuovo DiskHeader
			nuovoDiskHeader = (DiskHeader*)malloc(sizeof(DiskHeader));
			// Devo inizializzare il DiskHeader e le BitMap (data e inode)
			nuovoDiskHeader->num_blocks = num_blocks;
			/// NUMERI INODE FISSO PER ORA
			// Quanti inode ho mappati sulla bitmap degli inode
			nuovoDiskHeader->inodemap_blocks = NUM_INODES * INODES_PER_BLOCK ;
			// Grandezza dell'inodemap in byte, arrotondati per eccesso con BLOCK_SIZE
			nuovoDiskHeader->inodemap_bytes = BLOCK_SIZE;
			
			// 3 blocchi sono riservati al superblocco e le 2 bitmap (data e inodes), oltre ai 5 riservati agli inode
			nuovoDiskHeader->bitmap_blocks = num_blocks - 3 - nuovoDiskHeader->inodemap_blocks/INODES_PER_BLOCK;
			nuovoDiskHeader->bitmap_bytes = BLOCK_SIZE;
			
			// Nuovo file tutti i blocchi/inode sono vuoti
			nuovoDiskHeader->dataFree_blocks = nuovoDiskHeader->bitmap_blocks;
			nuovoDiskHeader->dataFirst_free_block = 0;
			nuovoDiskHeader->inodeFree_blocks = nuovoDiskHeader->inodemap_blocks;
			nuovoDiskHeader->inodeFirst_free_block = 0;
						
			// Mmap del DiskHeader e delle BitMap ( Data e Inodes )
			// Mmap del diskheader all'inizio del file
			fprintf(stderr, "BlockSize: %d\n", BLOCK_SIZE);

			// Mmappo i primi 3 blocchi del file, in quanto l'offset è una truffa
			zonaMappata = mmap(NULL, 3*BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			if ( zonaMappata != MAP_FAILED ) 
			{
				diskDriver->header = (DiskHeader*)zonaMappata;
				//fprintf(stderr, "Header: %s\n", (char*)diskDriver->header);
				diskDriver->bitmap_data_values = (char*)(zonaMappata+BLOCK_SIZE);
				diskDriver->bitmap_inode_values = (char*)(zonaMappata+(2*BLOCK_SIZE));
				diskDriver->fd = fd;
				fprintf(stderr, "FD: %d\n", fd);
				fprintf(stderr, "FD in diskdriver: %d\n", fd);
				// Copio il nuovo DiskHeader creato prima
				memcpy(diskDriver->header, nuovoDiskHeader, sizeof(DiskHeader)); //???? -> scrivo il diskheader creato prima nella zona mmappata e successivamente flush
				DiskDriver_flush(diskDriver);
			}
			else
			{
				perror("Errore durante la mmap di tutta la prima parte del file (nuovo file)\n");
				exit(1);
			}
			
			// BitMap bitmapData = BitMap_init();
			// Non faccio l'inizializzazione della bitmap perché è già tutto a 0 >.<
			
			// Scrivo le strutture sul file, flush mmap
			//DiskDriver_flush(diskDriver);
		}
		else
		{
			perror("Errore durante la creazione del file\n");
			exit(1);
		}
	}
}
	
// reads the block in position block_num
// returns -1 if the block is free accrding to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num) {
	
	
	block_num= NUM_SUPER+NUM_BITMAPS+NUM_INODES;
	char* bm=disk->bitmap_data_values;
	
	BitMapEntryKey k=BitMap_blockToIndex(block_num);

	
	if( bit_get(bm[k.entry_num],k.bit_num)==0 ){	//intere block (8 celle) all zeros
		perror("cannot read cause block in bitmap is zero");
		return -1;
	}

	void* blockRead=(void*) malloc(BLOCK_SIZE);
	//move in the file till block num reached
	if(lseek(disk->fd, block_num*BLOCK_SIZE, SEEK_SET) < 0 ){
		perror("ERR:");
		exit(1);
	}
	
	//TODO last accessed in inode has to be updated
	if(read(disk->fd, blockRead, BLOCK_SIZE) < 0 ){
		perror("ERR:");
		exit(1);
	}
	
	memcpy(dest, blockRead, BLOCK_SIZE);
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
	block_num= NUM_SUPER+NUM_BITMAPS+NUM_INODES;
	char* bm= disk->bitmap_data_values;
	BitMapEntryKey k= BitMap_blockToIndex(block_num);

	//TODO modify the bitmap
	if( bit_get(bm[k.entry_num],k.bit_num)==0 ){	//intere block (8 celle) all zeros
		perror("cannot write cause block not allocated");
		return -1;
	}
	
	if(lseek(disk->fd, block_num * BLOCK_SIZE, SEEK_SET) < 0 ){
		perror("ERR:");
		exit(1);
	}
	
	//TODO last accessed in inode has to be updated
	if(write(disk->fd, src, BLOCK_SIZE) < 0 ){
		perror("ERR:");
		exit(1);
	}
	return 0;
}

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, int block_num)
{
	int ret = -1;
	BitMapEntryKey k;
	BitMap* bitmap;
	
	// Verifica che il blocco da liberare esista	
	if ( block_num < disk->header->bitmap_blocks )
	{
		//k = BitMap_blockToIndex(block_num);
		// Setto nella bitmap che il blocco nel dato indice è libero
		bitmap = malloc(sizeof(BitMap));
		bitmap->num_bits = disk->header->bitmap_blocks;
		bitmap->entries = disk->bitmap_data_values;
		if ( BitMap_set(bitmap, block_num, 0) != -1 )
		{
			// Modifico le informazioni relative ai blocchi data liberi 
			// memorizzati nel DiskHeader 
			disk->header->dataFree_blocks = disk->header->dataFree_blocks + 1;
			if ( block_num < disk->header->dataFirst_free_block ) 
			{
				disk->header->dataFirst_free_block = block_num;
			}	
			// Ritorno valore diverso da -1 	
			ret = 0;
		}
		// Libero la struttura allocata dinamicamente usata
		free(bitmap);
	}
	
	return ret;
}

// returns the first free block in the disk from position (checking the bitmap)
int DiskDriver_getFreeBlock(DiskDriver* disk, int start)
{
	int index;
	BitMap* bitmap;
	// Se la posizione di partenza è fuori range oppure non ci sono blocchi liberi...
	if ( start < 0 || start > disk->header->bitmap_blocks || disk->header->dataFree_blocks == 0 )
	{
		// ... errore -1
		index = -1;
	}
	// Se si vuole il primo blocco data libero...
	else if ( start == 0 )
	{
		// ...Lo si ottiene dal DiskHeader
		index = disk->header->dataFirst_free_block;
	}
	else
	{
		// Altrimenti ricerca nella bitmap 
		bitmap = (BitMap*)malloc(sizeof(BitMap));
		index = BitMap_get(bitmap, start, 0);
		free(bitmap);
	}
	
	return index;
}

// writes the data (flushing the mmaps)
// Returns -1 if operation not possible, 0 if successfull
int DiskDriver_flush(DiskDriver* disk)
{
	int ret = -1;
	
	if ( msync(disk->header, 3*BLOCK_SIZE, MS_SYNC) != -1 )
	{
		ret = 0;
	}

	return ret;
}

