#include "simplefs.h"

#define DEBUG 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int main(int agc, char** argv) {
  //#if DEBUG
	  // prova di linking TODO da rimuovere 
	  printf("\t\tINIZIO DEBUGGING FUNZIONI...\n");
	 

	int ret;
	
	//-------------------BITMAP TESTING-----------------------------------
	
	printf("\t--------------| DEBUGGING BITMAP |----------------------: \n");


	// converts a bit to a linear index
	int r1=BitMap_indexToBlock(2, 7);
	printf("BitMap_indexToBlock(2, 7): %d \n ",r1);
	r1=BitMap_indexToBlock(2, 8);
	printf("BitMap_indexToBlock(2, 8): %d \n ",r1);


	printf("Prova Bitmap vuote \n");
	BitMap bm =BitMap_init();	//inizializza la bitmap, creando array (vuoto)
	bits_print(bm.entries,10);
	
	BitMap* bmap = (BitMap*)malloc(sizeof(BitMap));
	memcpy(bmap,&bm,sizeof(BitMap));
	bits_print( bmap->entries,10);

	//char* cc=calloc(sizeof(char),100);
	//strcpy(cc,"");
	//bits_print(cc,5);
	// sets the bit at index pos in bmap to status
	r1= BitMap_set(bmap, 270, 1);
	printf(" BitMap_set(bmap, 270, 1): %d \n ",r1);

	r1= BitMap_set(bmap, 2, 1);
	printf("set %d \n ",r1);

	bits_print(bmap->entries,10);

	r1=BitMap_get( bmap, 270, 1);
	printf("get %d \n ",r1);

	r1=BitMap_get( bmap, 0, 1);
	printf("get %d \n ",r1);

	BitMapEntryKey k = BitMap_blockToIndex(3);
	printf("%d %d \n",k.entry_num,k.bit_num);


	//-------------------DISK_DRIVER TESTING-----------------------------------

	printf("\t-----------------| DEBUGGING DISK_DRIVER |------------------------: \n");
	//printf("\t init: void\n"); DiskDriver_init(NULL,"fileTest.txt",2);
	/*
	printf("\t readBlock: void\n"); DiskDriver_readBlock(disk, void* dest, int block_num);
	printf("\t writeBlock: void\n");DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num);
	printf("\t freeBlock: void\n"); DiskDriver_freeBlock(DiskDriver* disk, int block_num);
	printf("\t getFreeBlock: void\n"); DiskDriver_getFreeBlock(DiskDriver* disk, int start);
	printf("\t flush: void\n"); DiskDriver_flush(DiskDriver* disk);
	*/
	DiskDriver* dd = (DiskDriver*) malloc(sizeof(DiskDriver));
	DiskDriver_init(dd,"fileTest",50);
	printf("--DISKHEADER--\n");
	//printf("Test done on an expected new file, please delete the file created after each run\n");
	printf("File descriptor: %d\n", dd->fd);
	printf("Numero blocchi totali: %d\n", dd->header->num_blocks);
	printf("Numero blocchi mappati dalla bitmap_data: %d\n", dd->header->bitmap_blocks);
	printf("Grandezza in byte della bitmap_data: %d\n", dd->header->bitmap_bytes);
	printf("Numero blocchi mappati dalla bitmap_inode: %d\n", dd->header->inodemap_blocks);
	printf("Grandezza in byte della bitmap_inode: %d\n", dd->header->inodemap_bytes);
	printf("Numero blocchi data liberi: %d\n", dd->header->dataFree_blocks);
	printf("Indice primo blocco data libero: %d\n", dd->header->dataFirst_free_block);
	printf("Numero blocchi inode liberi: %d\n", dd->header->inodeFree_blocks);
	printf("Indice primo blocco inode libero: %d\n", dd->header->inodeFirst_free_block);
	printf("Bitmap data: \n");
	bits_print(dd->bitmap_data_values, 10);
	printf("Bitmap inode: \n");
	bits_print(dd->bitmap_inode_values, 10);
	
	//int i;

	BitMap* bm2=malloc(sizeof(BitMap));
	bm2->entries= dd->bitmap_data_values;
	bm2->num_bits= dd->header->bitmap_bytes*8;
	BitMap_set(bm2,2,1);
	memcpy(dd->bitmap_data_values,bm2->entries,256);
	
		int i,j;
	for(i=0;i< 5;i++){
		for(j=0;j<8;j++) bit_set(bm2->entries[i],j);
		j=0; 
		}
	memcpy(dd->bitmap_data_values,bm2->entries,256);

	bits_print(dd->bitmap_data_values,10);
	
	printf("Numero blocchi mappati dalla bitmap_inode: %d\n", dd->header->inodemap_blocks);
	printf("call write on 2: %d \n", DiskDriver_writeBlock(dd,"lorem ipsum",2) ); 

	char* sread=malloc(sizeof(BLOCK_SIZE));
	printf("call read on 1: %d\n",DiskDriver_readBlock(dd,sread,2));
	printf("readed: %s\n",sread);
	//free(sread);
	char* blockw = malloc(sizeof(BLOCK_SIZE));
	printf("Numero blocchi mappati dalla bitmap_inode: %d\n", dd->header->inodemap_blocks);
	printf("Test freeBlock() e getFreeBlock()\n");
	BitMap* bitmapTest = (BitMap*)malloc(sizeof(BitMap));
	bitmapTest->num_bits = dd->header->bitmap_blocks;
	bitmapTest->entries = dd->bitmap_data_values;
	printf("Set data block in index 0 to 1 (value expected 1)=> %d\n", BitMap_set(bitmapTest, 0, 1));
	printf("Get first free data block (value expected 1) => %d\n", DiskDriver_getFreeBlock(dd, 0));
	printf("Free data block in index 0 (value expected 0) => %d\n", DiskDriver_freeBlock(dd, 0));
	printf("Free data block in index out of range (value expected -1) => %d\n", DiskDriver_freeBlock(dd, 113133));
	printf("Get first free data block (value expected 0) => %d\n", DiskDriver_getFreeBlock(dd, 0));
	//printf("Lo si prende dal disk_header => %d \n",dd->header->dataFirst_free_block);
	printf("Numero blocchi mappati dalla bitmap_inode: %d\n", dd->header->inodemap_blocks);
	printf("--DISKDRIVER END--\n");

	//-------------------SIMPLE_FS TESTING-----------------------------------

	printf("\t--------------------| DEBUGGING simplefs |------------------------: \n");
	printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
	printf("DataBlock size %ld\n", sizeof(FileBlock));
	printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
	printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));
	SimpleFS* fileSystem = (SimpleFS*) malloc(sizeof(SimpleFS));
	fileSystem->disk = dd;
	printf("Before format max numbers inode %d\n", fileSystem->disk->header->inodemap_blocks);
	printf("Format the disk\n");
	SimpleFS_format(fileSystem);
	printf("After format max numbers inode %d\n", fileSystem->disk->header->inodemap_blocks);
	printf("And then init the filesystem\n");
	DirectoryHandle* toplevel = SimpleFS_init(fileSystem, dd);
	printf("Directory Handle toplevel\n");
	printf("Number entities: %d\n", toplevel->fdb->num_entries);
	printf("Name of folder: %s\n", toplevel->fdb->fcb.name);
	printf("Create new file called pippo.txt, wow.txt, pippo.txt, testare.al and directory hello\n");
	FileHandle* hf=malloc(sizeof(hf));
	hf=SimpleFS_createFile(toplevel, "pippo.txt");
	SimpleFS_createFile(toplevel, "pippo.txt");
	/*if ( hf == NULL )
	{
		printf("Errore durante la creazione del file\n");
	}*/
	SimpleFS_createFile(toplevel, "wow.txt");
	SimpleFS_createFile(toplevel, "testare.al");
	SimpleFS_mkDir(toplevel, "directory");
	i=0;
	//while ( i < 
	
	char** contenutoDirectory = (char**)malloc(toplevel->fdb->num_entries*sizeof(char*));
	i = 0;
	while ( i < toplevel->fdb->num_entries )
	{
		contenutoDirectory[i] = (char*)malloc(128*sizeof(char));
		i++;
	}
	printf("Test readDir() in toplevel\n");
	SimpleFS_readDir(contenutoDirectory, toplevel);
	i = 0;
	printf("How many files in toplevel directory: %d\n", toplevel->fdb->num_entries);
	while ( i < toplevel->fdb->num_entries )
	{
		printf("Index %d : name = %s\n",i, contenutoDirectory[i]);
		i++;
	}
	
	printf("Number max inode in FirstDirectoryBlock: %ld\n", (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)-sizeof(int))/sizeof(int));
	if(hf==NULL)
		perror("hf null");
	//hf=SimpleFS_openFile(toplevel,"pippo.txt");
	if(hf==NULL)
		perror("hf null");
	//printf("Text of simplefs_write: %d \n",SimpleFS_write(hf,"writingTextString007",20) );
	char buf[20];
	//printf("Text of simplefs_read: %d \n",SimpleFS_read(hf,buf,20) );
	
	ret=SimpleFS_seek(hf, 6);
	if(ret<0) perror("err ret of seek \n");


	printf("Mkdir: paolino => %d\n", SimpleFS_mkDir(toplevel, "paolino"));
	printf("Changedir in nuovaDir %d\n", SimpleFS_changeDir(toplevel, "nuovaDir"));
	printf("Changedir in paolino %d\n", SimpleFS_changeDir(toplevel, "paolino"));
	printf("Changedir in toplevel %d\n",SimpleFS_changeDir(toplevel, "paolino"));
	//ret=SimpleFS_changeDir(toplevel, "directory");
	//if(ret<0) perror("err ret of changeDir \n");

	//ret=SimpleFS_remove(fileSystem, "wow.txt");
	//if(ret<0) perror("err ret of remove \n");
	
	//ret=SimpleFS_changeDir(toplevel, "directory");
	//if(ret<0) perror("err ret of changeDir \n");

	//fileSystem->current_directory_inode=toplevel->inode;
	
	ret=SimpleFS_listFiles(fileSystem);
	if(ret<0) perror("err ret of list files");
	
	/*
	ret=SimpleFS_remove(fileSystem, "wow.txt");
	if(ret<0) perror("err ret of remove \n");
	
	ret=SimpleFS_remove(fileSystem, "paolino");
	if(ret<0) perror("2: err ret of remove \n");
	
	*/
	ret=SimpleFS_listFiles(fileSystem);
	if(ret<0) perror("2: err ret of list files");
	
/*
	ret=SimpleFS_close(hf);
	if(ret<0) perror("err ret of close \n");
 */
 
 // #endif
 
}
