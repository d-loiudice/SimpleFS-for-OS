#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "disk_driver.h"
#include "simplefs.h"

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
			directoryHandle->parent = NULL;
			//directoryHandle->dcb = (FirstDirectoryBlock*) malloc(sizeof(DirectoryHandle));
			// La top level directory è registrata nel primo inode
			// Ottengo il primo inode e da lì ottengo l'indice del blocco che contiene
			// il FirstDirectoryBlock
			// Dall'inode ottengo l'indice del blocco contenente il FirstDirectoryBlock
			directoryHandle->dcb = (FirstDirectoryBlock*) malloc(sizeof(FirstDirectoryBlock));
			// L'indice del blocco data contenente i dati è nell'inode
			if ( DiskDriver_readBlock(disk, (void*)directoryHandle->dcb, inode->primoBlocco) != -1 )
			{
				// Setto i restanti campi della directory handle
				directoryHandle->current_block = &(directoryHandle->dcb->header); // Sono nel primo blocco
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
			firstDirectoryBlock->fcb.directory_block = -1; // Cartella principale, no parent
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
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename);

// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names, DirectoryHandle* d);


// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename);


// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f);

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size);

// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size);

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
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname);

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(SimpleFS* fs, char* filename);


  

