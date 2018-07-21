#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>
#include "disk_driver.h"
#include "simplefs.h"

#define TRUE 1
#define FALSE 0

// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk)
{
	DirectoryHandle* directoryHandle = NULL;
	Inode* inode;
	BitMap* bitmapInode;
	//BitMap* bitmapData;
	//FirstDirectoryBlock* firstDirectoryBlock;
	// Verifica che ci sia una struttura allocata
	if ( disk != NULL ) 
	{
		// Verifica che nella bitmap degli inode sia occupato il primo blocco inode (indice 0 )
		bitmapInode = (BitMap*)malloc(sizeof(BitMap));
		bitmapInode->num_bits = disk->header->inodemap_blocks;
		bitmapInode->entries = disk->bitmap_inode_values;
		//bitmapData = (BitMap*)malloc(sizeof(BitMap));
		//bitmapData->num_bits = disk->header->bitmap_blocks;
		//bitmapData->entries = disk->bitma
		fprintf(stderr, "Valore ottenuto da BitMapInode_get: %d\n", BitMap_get(bitmapInode, 0, 1));
		inode = (Inode*)malloc(sizeof(Inode));
		// Leggo il primo inode
		if ( DiskDriver_readInode(disk, inode, 0) != -1 )
		{
			fprintf(stderr, "Ho letto il primo inode\n");
			fs->disk = disk;
			directoryHandle = (DirectoryHandle*) malloc(sizeof(DirectoryHandle));
			directoryHandle->sfs = fs;
			directoryHandle->pos_in_dir = 0;
			directoryHandle->pos_in_block = 0; 
			//directoryHandle->parent_inode = -1;
			directoryHandle->inode = 0;
			//directoryHandle->dcb = (FirstDirectoryBlock*) malloc(sizeof(DirectoryHandle));
			// La top level directory è registrata nel primo inode
			// Ottengo il primo inode e da lì ottengo l'indice del blocco che contiene
			// il FirstDirectoryBlock
			// Dall'inode ottengo l'indice del blocco contenente il FirstDirectoryBlock
			directoryHandle->fdb = (FirstDirectoryBlock*) malloc(sizeof(FirstDirectoryBlock));
			// L'indice del blocco data contenente i dati è nell'inode
			if ( DiskDriver_readBlock(disk, (void*)directoryHandle->fdb, inode->primoBlocco) != -1 )
			{
				// Setto i restanti campi della directory handle
				directoryHandle->current_block = &(directoryHandle->fdb->header); // Sono nel primo blocco
			}
			free(inode);
		}
		else
		{
			fprintf(stderr, "Errore durante la lettura del primo inode\n");
		}
		free(bitmapInode);
	}
	
	return directoryHandle;
}

// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void SimpleFS_format(SimpleFS* fs)
{
	BitMap* bitmapInode;
	Inode* inode;
	FirstDirectoryBlock* firstDirectoryBlock;
	// fs inizializzata con il DiskDriver prima della chiamata di questa funzione
	if ( fs != NULL && fs->disk != NULL )
	{
		// Pulisco la bitmap dei dati e degli inode
		memset(fs->disk->bitmap_data_values, 0, BLOCK_SIZE);
		memset(fs->disk->bitmap_inode_values, 0, BLOCK_SIZE);
		bitmapInode = (BitMap*) malloc(sizeof(BitMap));
		bitmapInode->num_bits = fs->disk->header->inodemap_blocks;
		bitmapInode->entries = fs->disk->bitmap_inode_values;
		// Creo l'inode per la cartella principale
		inode = (Inode*) malloc(sizeof(Inode));
		inode->permessiUtente = 7;
		inode->permessiGruppo = 7;
		inode->permessiAltri = 7;
		inode->idUtente = 0;
		inode->idGruppo = 0;
		inode->dataCreazione = time(NULL);
		inode->dataUltimoAccesso = inode->dataCreazione;
		inode->dataUltimaModifica = inode->dataCreazione; // E' stato creato ora quindi sono uguali
		inode->dimensioneFile = 4096; // cartella
		inode->dimensioneInBlocchi = 1;
		inode->tipoFile = 'd';
		inode->primoBlocco = 0; // Primo indice data
		// Scrittura dell'inode su file
		//if ( lseek(fs->disk->fd, BLOCK_SIZE+fs->disk->header->bitmap_bytes+fs->disk->header->inodemap_bytes, SEEK_SET) != -1 )
		if ( DiskDriver_writeInode(fs->disk, inode, 0) != -1 )
		{
			// Setto l'indice dell'inode contenente le informazioni della cartella principale
			fs->current_directory_inode = 0;
			// Creazione blocco data per la cartella principale (root)
			firstDirectoryBlock = (FirstDirectoryBlock*) malloc(sizeof(FirstDirectoryBlock));
			firstDirectoryBlock->header.block_in_file = 0;
			firstDirectoryBlock->header.next_block = -1; // Non ci sono né blocchi precedenti né successivi, in quanto appena creato
			firstDirectoryBlock->header.previous_block = -1;
			strcpy(firstDirectoryBlock->fcb.name, "/");
			firstDirectoryBlock->fcb.block_in_disk = 0;
			//firstDirectoryBlock->fcb.directory_block = -1; // Cartella principale, no parent
			firstDirectoryBlock->fcb.parent_inode = -1;
			firstDirectoryBlock->num_entries = 0;
			// Inizializzazione array della cartella per indicare gli inode figli contenenti in essa
			int dim = (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)-sizeof(int))/sizeof(int);
			int index = 0;
			while ( index < dim )
			{
				firstDirectoryBlock->file_inodes[index] = -1;
				index = index + 1;
			}
			// Scrittura del fcb su file nella posizione 0 dei data blocks
			if ( DiskDriver_writeBlock(fs->disk, (void*)firstDirectoryBlock, 0) != -1 )
			{
				// OK
			}
			else
			{
				perror("Errore durante la scrittura del first directory block principale (root)\n");
			}
			free(firstDirectoryBlock);
		}
		else
		{
			perror("Errore durante la scirttura dell'inode principale (root)\n");
		}
		DiskDriver_flush(fs->disk);
		free(inode);
		free(bitmapInode);
	}
}

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks inodes and datas)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename)
{
	FileHandle* fileHandle = NULL;
	BitMap* bitmapInode;
	BitMap* bitmapData;
	FirstFileBlock* firstFileBlock;
	Inode* inode;
	int indexInode;
	int indexData;
	bitmapInode = (BitMap*)malloc(sizeof(BitMap));
	bitmapData = (BitMap*)malloc(sizeof(BitMap));
	bitmapInode->num_bits = d->sfs->disk->header->inodemap_blocks;
	bitmapInode->entries = d->sfs->disk->bitmap_inode_values;
	bitmapData->num_bits = d->sfs->disk->header->bitmap_blocks;
	bitmapData->entries = d->sfs->disk->bitmap_data_values;
	// Verifica che ci sia un inode libero
	if ( d->sfs->disk->header->inodeFree_blocks > 0 )
	{
		indexInode = d->sfs->disk->header->inodeFirst_free_block;
		// Verifica che ci sia un blocco data libero
		if ( d->sfs->disk->header->dataFree_blocks > 0 )
		{
			// Ottengo il blocco data dove andrò a memorizzare il file
			indexData = d->sfs->disk->header->dataFirst_free_block;
			
			// Inizializzazione inode
			inode = (Inode*)malloc(sizeof(Inode));
			inode->permessiAltri = 7;
			inode->permessiUtente = 7;
			inode->permessiGruppo = 7;
			inode->idUtente = 0;
			inode->idGruppo = 0;
			inode->dataCreazione = time(NULL);
			inode->dataUltimoAccesso = inode->dataCreazione;
			inode->dataUltimaModifica = inode->dataCreazione;
			inode->dimensioneFile = 0;
			inode->dimensioneInBlocchi = 1;
			inode->tipoFile = 'r';
			inode->primoBlocco = indexData;
			
			// Inizializzazione FirstFileBlock
			firstFileBlock = (FirstFileBlock*)malloc(sizeof(FirstFileBlock));
			// BlockHeader
			firstFileBlock->header.block_in_file = 0;
			firstFileBlock->header.next_block = -1;
			firstFileBlock->header.previous_block = -1;
			// FCB
			firstFileBlock->fcb.parent_inode = d->inode;
			firstFileBlock->fcb.block_in_disk = indexData;
			strncpy(firstFileBlock->fcb.name, filename, 128);
			memset(firstFileBlock->data, 0, BLOCK_SIZE-sizeof(FileControlBlock) - sizeof(BlockHeader));
			
			// Scrittura delle strutture dati
			if ( DiskDriver_writeInode(d->sfs->disk, inode, indexInode) != -1 ) 
			{
				if ( DiskDriver_writeBlock(d->sfs->disk, (void*)firstFileBlock, indexData) != -1 )
				{
					// OK
					fileHandle = (FileHandle*) malloc(sizeof(FileHandle));
					fileHandle->sfs = d->sfs;
					fileHandle->ffb = firstFileBlock;
					//fileHandle->parent_inode = 
					fileHandle->inode = indexInode;
					fileHandle->current_block = &(firstFileBlock->header);
					fileHandle->pos_in_file  = 0;
				}
			}
			else
			{
				fprintf(stderr, "Errore durante la scrittura dell'inode\n");
			}
		}
	}
	
	return fileHandle;
}
// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names, DirectoryHandle* d);


// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename);


// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f){
	if (f==NULL) {
		perror("file not valid (null)");
		return -1;
	}
	free(f->current_block);
	free(f->ffb);
	free(f);
	printf("ciaone\n");
	return 1;

}

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size){
	int ret;
	if(f->ffb==NULL){	//un po come se il file fosse vuoto 
		perror("cannot find first file block");
		return -1;
	}
	
	
	//lettura inode
	Inode* in_read=malloc(sizeof(Inode));
	int in_block_num=f->inode ; //index to access the inode of the file 
	
	if(DiskDriver_readInode(f->sfs->disk,in_read,in_block_num ) < 0 ){
		perror("errore read inode for write");
		return -1;
	}
	if(in_read->tipoFile != 'r'){	//must be a file to write on
		perror("not valid tipoFile");
		return -1;
		}
	
	
	//scrittura su blocco
	
	int limit;	//quanto puo entrare nel blocco considerando l header 
	unsigned char is_ffb;	//boolean per sapere se siamo nel primo blocco
	if(f->current_block->block_in_file==0){ //abbiamo un fcb e siamo quindi in ffb
		limit=BLOCK_SIZE - sizeof(BlockHeader)-sizeof(FileControlBlock);
		is_ffb=TRUE;
	}
	else{
		limit=BLOCK_SIZE - sizeof(BlockHeader);
		is_ffb=FALSE;
	}
	
	FileBlock* fbw= malloc(sizeof(FileBlock)); 	//alloco blocco che scriverò
	FirstFileBlock* ffbw= malloc(sizeof(FirstFileBlock));;
	void* dest; 
	char* d_data;
	if(is_ffb){	//parto dal primo blocco
		dest=ffbw;
		d_data=ffbw->data;
		ffbw=f->ffb;	//prendo direttamente il ffb dall handle
		}
	
	else{	//non parto dal primo blocco
		dest=fbw;
		d_data=fbw->data;
		fbw->header= *(f->current_block);
	}
	
	int num_blocks_to_write=0; //num blocchi su cui scriverò
	int data_block_num=f->current_block->block_in_file;	//block num per la funz.
	int bytes_written=0;
	
	
	//Se mi basta sostituire un blocco solo
	if(size <= limit ){	

		memcpy(d_data,data,size);
		if(DiskDriver_writeBlock(f->sfs->disk,dest,data_block_num) < 0){
			perror("cannot write block");
			return -1;
		}
		bytes_written=size;
	}
	
	
	//Se ho più blocchi su cui devo scrivere (almeno 2)
	else{	
		
		if(is_ffb)
			num_blocks_to_write= ceil( (double)( size-(BLOCK_SIZE - sizeof(FileControlBlock)-sizeof(BlockHeader) ) )\
			/(BLOCK_SIZE- sizeof(BlockHeader)) ) + 1;	//conto i sotto blocchi
		else
			num_blocks_to_write= size/(BLOCK_SIZE-sizeof(BlockHeader) )+ 1;	//conto i sotto blocchi
		int new_size=BLOCK_SIZE-sizeof(BlockHeader);	//devo scrivere l intero blocco
		int i=0;
		while(i < num_blocks_to_write){
			if(i == num_blocks_to_write-1) // ultimo blocco da scrivere
				new_size=size - bytes_written;
			
			//carico il blocco, mi muovo di new_size in new_size sul data
			//fornito e scrivo sempre new_size su fwb->data tranne per ultimo blocco
			if(DiskDriver_writeBlock(f->sfs->disk,dest,data_block_num) < 0){
				perror("cannot write block");
				return -1;
			}
			memcpy(d_data,data+bytes_written,new_size);

			bytes_written+=new_size;
			if(i == num_blocks_to_write-1) break;	//ho scritto l ultimo
			
			//preparo il blockHeader per il prossimo blocco
			FileBlock* fbr=malloc(sizeof(FileBlock));
			ret= DiskDriver_readBlock(f->sfs->disk,fbr,
						f->current_block->next_block);	//file block letto
			if(ret< 0){
				perror("errore in readblock di write simple fs");
				return -1;
				}
			
			memcpy(&fbw->header,&fbr->header,sizeof(BlockHeader)); //ho preso l head del blocco successivo
			
			free(fbr);	//free di fbr puo causare  seg_fault
			data_block_num=fbr->header.block_in_file;
			
			i++;
			}
		}
	
		//aggiornamento inode
		
		in_read->dataUltimaModifica=time(NULL);
		ret= DiskDriver_writeInode(f->sfs->disk,in_read,f->inode);
		if(ret < 0){
			perror("erorre write inode in write simplefs");
			return -1;
			}
			
		free(in_read);
		free(ffbw);
		free(fbw); 
		
		return bytes_written;
	}






// read in the file, at current position size bytes and store them in data
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size){
	
	
	
	//read the inode info of the file
	Inode* in_read=malloc(sizeof(Inode));
	int in_block_num=f->inode ; //index for inode block
	
	if(DiskDriver_readInode(f->sfs->disk,in_read,in_block_num ) < 0 ){
		perror("errore read inode for read");
		return -1;
	}
	if(in_read->tipoFile != 'r'){
		perror("not valid tipoFile");
		return -1;
		}
		
	//read the file blocks of the file
	int limit,ret;
	unsigned char is_ffb;
	void* dest;
	char* d_data;
	FileBlock* fb_to_r= malloc(sizeof(FileBlock));
	FirstFileBlock* ffb_to_r= malloc(sizeof(FirstFileBlock));
	if(f->current_block->block_in_file==0){
		is_ffb=TRUE;
		limit=BLOCK_SIZE - sizeof(BlockHeader)-sizeof(FileControlBlock);
		dest=ffb_to_r;
		d_data=ffb_to_r->data;
	 }
	else{
		is_ffb=FALSE;
		limit=BLOCK_SIZE - sizeof(BlockHeader);
		dest=fb_to_r;
		d_data=fb_to_r->data;
	}
	
	int data_block_num=f->current_block->block_in_file;
	int bytes_read=0;	
	
	// Se mi basta leggere in un solo blocco 
	int num_blocks_to_read=1;
	if(size <= limit){
		if(DiskDriver_readBlock(f->sfs->disk,dest,data_block_num) < 0){
			perror("cannot read block");
			return -1;
		}
		memcpy(data,d_data,size);
		bytes_read = size;
	}
	
	// Se ho piu blocchi da dover leggere
	else{
		if(is_ffb)
			num_blocks_to_read= ceil((double) ( size-(BLOCK_SIZE - sizeof(FileControlBlock)-sizeof(BlockHeader) ) )\
			/(BLOCK_SIZE- sizeof(BlockHeader)) ) + 1;
		else
			num_blocks_to_read= size/(BLOCK_SIZE-sizeof(BlockHeader) )+ 1;	//conto i sotto blocchi
		
		int new_size=BLOCK_SIZE-sizeof(BlockHeader);	//devo scrivere l intero blocco
		int i=0;
		while(i < num_blocks_to_read){
			if(i == num_blocks_to_read-1) // ultimo blocco da scrivere
				new_size=size - bytes_read;
			
			//carico il blocco, mi muovo di new_size in new_size sul data
			//fornito e scrivo sempre new_size su fwb->data tranne per ultimo blocco
			
			if(DiskDriver_readBlock(f->sfs->disk,dest,data_block_num) < 0){
				perror("cannot write block");
				return -1;
			}
			memcpy(data+bytes_read,d_data,new_size);
			
			bytes_read+=new_size;
			if(i == num_blocks_to_read-1) break;	//ho scritto l ultimo
			
			//preparo il blockHeader per il prossimo blocco
			FileBlock* fbr=malloc(sizeof(FileBlock));
			ret= DiskDriver_readBlock(f->sfs->disk,fbr,
						f->current_block->next_block);	//file block letto
			if(ret< 0){
				perror("errore in readblock di write simple fs");
				return -1;
				}
			
			memcpy(&fb_to_r->header,&fbr->header,sizeof(BlockHeader)); //ho preso l head del blocco successivo
			
			free(fbr);	//free di fbr puo causare  seg_fault
			data_block_num=fbr->header.block_in_file;
			
			i++;
			}
		}
	
	
	//data_block_num=f->current_block->next_block;
	//if(data_block_num < 0)	//reached end of the file

	
	
	//update inode info of the file
	in_read->dataUltimoAccesso=time(NULL);	//update ultimo accesso
	if(DiskDriver_writeInode(f->sfs->disk,in_read,in_block_num) <0){
		perror("updating inode failed");
		return -1;
	}
	
	free(in_read);
	free(ffb_to_r);
	free(fb_to_r);
	
	
	return bytes_read;
	}

// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int SimpleFS_seek(FileHandle* f, int pos);

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
int SimpleFS_changeDir(DirectoryHandle* d, char* dirname);

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname) {
	BitMap* bitmapInode=malloc(sizeof(BitMap));
	BitMap* bitmapData=malloc(sizeof(BitMap));
	bitmapInode->num_bits = d->sfs->disk->header->inodemap_blocks;
	bitmapInode->entries = d->sfs->disk->bitmap_inode_values;
	bitmapData->num_bits = d->sfs->disk->header->bitmap_blocks;
	bitmapData->entries = d->sfs->disk->bitmap_data_values;
	Inode* inode;
	int indexInode;
	int indexData;
	FirstDirectoryBlock* firstDirectoryBlock;
	if ( d->sfs->disk->header->inodeFree_blocks > 0 ) {
		indexInode=d->sfs->disk->header->inodeFirst_free_block;
		if (d->sfs->disk->header->dataFree_blocks>0) {
			indexData= d->sfs->disk->header->dataFirst_free_block

			inode = (Inode*)malloc(sizeof(Inode));
			inode->permessiAltri = 7;
			inode->permessiUtente = 7;
			inode->permessiGruppo = 7;
			inode->idUtente = 0;
			inode->idGruppo = 0;
			inode->dataCreazione = time(NULL);
			inode->dataUltimoAccesso = inode->dataCreazione;
			inode->dataUltimaModifica = inode->dataCreazione;
			inode->dimensioneFile = 0;
			inode->dimensioneInBlocchi = 1;
			inode->tipoFile = 'd';
			inode->primoBlocco = indexData;

			firstDirectoryBlock=malloc(sizeof(FirstDirectoryBlock));

			firstDirectoryBlock->header.block_in_file=0;
			firstDirectoryBlock->header.next_block=-1;
			firstDirectoryBlock->header.previous_block=-1;
			
			FirstDirectoryBlock->num_entries=0;

			firstDirectoryBlock->fcb.parent_inode=d->inode;
			firstDirectoryBlock->fcb.block_in_disk=indexData;
			strncpy(firstDirectoryBlock->fcb.name, dirname, 128);
			memset(firstDirectoryBlock->file_inodes, -1, (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)-sizeof(int))/sizeof(int) );

			if ( DiskDriver_writeInode(d->sfs->disk, inode, indexInode)!= -1 ) {
				if (DiskDriver_writeBlock(d->sfs->disk, (void*)firstDirectoryBlock, indexData) !=-1) {
					return 0;


				}

			}

			else {
				fprintf(stderr, "Errore durante la scrittura dell'inode\n");
				return -1;
			}
			



		}

	}
	return -1; 

}

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(SimpleFS* fs, char* filename);


  

