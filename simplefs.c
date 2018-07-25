#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>
#include <assert.h>
#include "disk_driver.h"
#include "simplefs.h"


#define TRUE 1
#define FALSE 0
#define FILE_DIM 2048 //sono 2048 * 512 B = 1024 KB = 1 MB   

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
		//fprintf(stderr, "SimpleFS_init() -> inode: Valore ottenuto da BitMap_get: %d\n", BitMap_get(bitmapInode, 0, 1));
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
			else
			{
				fprintf(stderr, "SimpleFS_init() -> Errore durante la scrittura del primo blocco data\n");
			}
			free(inode);
		}
		else
		{
			fprintf(stderr, "SimpleFS_init() -> Errore durante la lettura del primo inode\n");
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
		//fprintf(stderr, "SimpleFS_format() -> valore inodemap_blocks = %d\n", fs->disk->header->inodemap_blocks);
		// Pulisco i valori nell'header del disco
		fs->disk->header->dataFree_blocks = fs->disk->header->bitmap_blocks;
		fs->disk->header->inodeFree_blocks = fs->disk->header->inodemap_blocks;
		fs->disk->header->inodeFirst_free_block = 0;
		fs->disk->header->dataFirst_free_block = 0;
		
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
		fprintf(stderr, "SimpleFS_format() -> dopo aver inizializzato inode : valore inodemap_blocks = %d\n", fs->disk->header->inodemap_blocks);
		// Scrittura dell'inode su file
		//if ( lseek(fs->disk->fd, BLOCK_SIZE+fs->disk->header->bitmap_bytes+fs->disk->header->inodemap_bytes, SEEK_SET) != -1 )
		if ( DiskDriver_writeInode(fs->disk, inode, 0) != -1 )
		{
			//fprintf(stderr, "SimpleFS_format() -> valore inodemap_blocks = %d\n", fs->disk->header->inodemap_blocks);
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
			fprintf(stderr, "SimpleFS_format() -> dopo aver inizializzato i file_inodes dentro firstdirectoryblock : valore inodemap_blocks = %d\n", fs->disk->header->inodemap_blocks);
			// Scrittura del fcb su file nella posizione 0 dei data blocks
			if ( DiskDriver_writeBlock(fs->disk, (void*)firstDirectoryBlock, 0) != -1 )
			{
				// OK
				fprintf(stderr, "SimpleFS_format() -> valore inodemap_blocks = %d\n", fs->disk->header->inodemap_blocks);
			}
			else
			{
				perror("SimpleFS_format() -> Errore durante la scrittura del first directory block principale (root)\n");
			}
			free(firstDirectoryBlock);
		}
		else
		{
			
			perror("SimpleFS_format() -> Errore durante la scrittura dell'inode principale (root)\n");
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
	int i = 0;
	int exist = 0;
	int dimensioneArray = d->fdb->num_entries;
	// Inizializzaizone per registrare i nomi della directory
	char** contenutoDirectory = (char**)malloc(sizeof(d->fdb->num_entries*sizeof(char*)));
	i = 0;
	while ( i < dimensioneArray )
	{
		contenutoDirectory[i] = (char*)malloc(128*sizeof(char));
		i++;
	}
	fprintf(stderr, "SimpleFS_createFile() -> allocato array con dimensione %d\n", dimensioneArray);
	SimpleFS_readDir(contenutoDirectory, d);
	i = 0;
	while ( i < dimensioneArray )
	{
		if (strncmp(filename, contenutoDirectory[i], 128) == 0 )
		{
			exist = 1;
		}
		i++;
	}
	bitmapInode = (BitMap*)malloc(sizeof(BitMap));
	bitmapData = (BitMap*)malloc(sizeof(BitMap));
	bitmapInode->num_bits = d->sfs->disk->header->inodemap_blocks;
	bitmapInode->entries = d->sfs->disk->bitmap_inode_values;
	bitmapData->num_bits = d->sfs->disk->header->bitmap_blocks;
	bitmapData->entries = d->sfs->disk->bitmap_data_values;
	if ( exist == 0 )
	{
		// Iterazione per verificare che ci sia 
		// Verifica che ci sia un inode libero
		if ( d->sfs->disk->header->inodeFree_blocks > 0 )
		{
			//indexInode = d->sfs->disk->header->inodeFirst_free_block; non aggiornati?
			indexInode = BitMap_get(bitmapInode, 0, 0);
			fprintf(stderr, "SimpleFS_createFile() -> blocchi data liberi: %d\n", d->sfs->disk->header->dataFree_blocks);
			// Verifica che ci sia un blocco data libero
			if ( d->sfs->disk->header->dataFree_blocks > 0 )
			{
				// Ottengo il blocco data dove andrò a memorizzare il file
				//indexData = d->sfs->disk->header->dataFirst_free_block; // non aggiornati?
				indexData = BitMap_get(bitmapData, 0, 0);
				fprintf(stderr, "Primo blocco data trovato libero: %d\n", indexData);
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

						// Inserisco l'inode nuovo nei file_inodes della cartella dove è stato creato il file
						if ( SimpleFS_insertInodeInDirectory(d, indexInode) != -1 )
						{
							// OK 
							// Aggiorno la struttura dati della cartella in cui ho creato il file
							d->fdb->num_entries = d->fdb->num_entries + 1;
							fprintf(stderr, "SimpleFS_createFile() -> Ora ci sono %d files\n", d->fdb->num_entries);
						}
						else
						{
							fprintf(stderr, "SimpleFS_createFile -> Impossibile inserire il nuovo inode nella directory\n");
						}
					}
					else
					{
						fprintf(stderr, "Errore durante la scrittura del blocco data\n");
					}
				}
				else
				{
					fprintf(stderr, "Errore durante la scrittura dell'inode\n");
				}
			}
			else
			{
				fprintf(stderr, "Non ci sono blocchi data disponibili\n");
			}
		}
		else
		{
			fprintf(stderr, "Non ci sono inode disponibili\n");
		}
	}
	else
	{
		fprintf(stderr, "SimpleFS_createFile -> File già esistente\n");
	}
	
	// Dealloco la struttura per memorizzare i nomi dei file contenuti nella cartella
	//char** contenutoDirectory = (char**)malloc(sizeof(d->fdb->num_entries*sizeof(char*)));
	fprintf(stderr, "SimpleFS_createFile() -> Inizio deallocazione array stringhe nomi file\n");
	i = 0;
	//int dimensione = ((fileHandle == NULL ) ? d->fdb->num_entries : d->fdb->num_entries-1);
	fprintf(stderr, "SimpleFS_createFile() -> Numero nomi file: %d\n", dimensioneArray);
	while ( i < dimensioneArray )
	{
		//fprintf(stderr, "SimpleFS_createFile() -> Dealloco posizione %d\n", i);
		free(contenutoDirectory[i]);
		//fprintf(stderr, "SimpleFS_createFile() -> Deallocato posizione %d\n", i);
		i++;
	}
	free(contenutoDirectory);
	fprintf(stderr, "SimpleFS_createFile() -> Fine deallocazione array stringhe nomi file\n");	
	return fileHandle;
}
// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names, DirectoryHandle* d)
{
	// names preallocato (d->fdb->num_entries) * 128
	// Inizializzazione per entrare nell'iterazione
	// int indexBlocks = 0;
	//BlockHeader* blockHeader = &(d->fdb->header)
	BlockHeader* blockHeader = (BlockHeader*)malloc(sizeof(BlockHeader));
	memcpy(blockHeader, &(d->fdb->header), sizeof(BlockHeader));
	int indexNames = 0; 		// Contatore per l'array dei nomi dei file
	int indexFiles;	    		// Contatore per l'array nel Directory Block
	int dimension;	    		// Dimensione dell'array nel Directory Block
	int indexBloccoAttuale;
	DirectoryBlock* directory;  // Cartella utilizzata per memorizzare i DirectoryBlock successivi
	Inode* inodeRead;
	void* blockRead;
	fprintf(stderr, "SimpleFS_readDir() -> numero files nella cartella = %d\n", d->fdb->num_entries);
	while ( blockHeader != NULL && indexNames < d->fdb->num_entries )
	{
		// Se è il FirstDirectoryBlock ho la possibilità di avere (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)-sizeof(int))/sizeof(int) files
		if ( blockHeader->block_in_file == 0 )
		{
			fprintf(stderr, "SimpleFS_readDir() -> sono nel FirstDirectoryBlock\n");
			dimension = (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)-sizeof(int))/sizeof(int);
			indexFiles = 0;
			// Ho già il FirstDirectoryBlock allocato
			while ( indexFiles < dimension )
			{
				// Se effettivamente è presente un indice inode valido relativo al file...
				if ( d->fdb->file_inodes[indexFiles] != -1 )
				{
					//fprintf(stderr, "SimpleFS_readDir() -> inode letto: %d\n", d->fdb->file_inodes[indexFiles]);
					//fprintf(stderr, "SimpleFS_readDir() -> indexFiles negli inode: %d\n", indexFiles);
					// ... Ottengo l'inode e successivamente il FirstFileBlock/FirstDirectoryBlock indicato dall'inode acquisito
					// e da esso leggo il nome del file contenuto nel FileControlBlock
					inodeRead = (Inode*)malloc(sizeof(Inode));
					if ( DiskDriver_readInode(d->sfs->disk, inodeRead ,d->fdb->file_inodes[indexFiles]) != -1 )
					{
						//fprintf(stderr, "Dimensione inode: %d", sizeof(Inode));
						fprintf(stderr, "SimpleFS_readDir -> letto dal disco l'inode\n");
						fprintf(stderr, "SimpleFS_readDir -> primo blocco dell'inode: %d\n", inodeRead->primoBlocco);
						//fprintf(stderr, "SimpleFS_readDir -> permessiUtente dell'inode: %d\n", inodeRead->permessiUtente);
						/*fprintf(stderr, "SimpleFS_readDir -> permessiGruppo dell'inode: %d\n", inodeRead->permessiGruppo);
						fprintf(stderr, "SimpleFS_readDir -> permessiAltri dell'inode: %d\n", inodeRead->permessiAltri);
						fprintf(stderr, "SimpleFS_readDir -> idutente dell'inode: %d\n", inodeRead->idUtente);
						fprintf(stderr, "SimpleFS_readDir -> idGruppo dell'inode: %d\n", inodeRead->idGruppo);
						fprintf(stderr, "SimpleFS_readDir -> dataCreazione dell'inode: %d\n", inodeRead->dataCreazione);
						fprintf(stderr, "SimpleFS_readDir -> dataUltimoAccesso dell'inode: %d\n", inodeRead->dataUltimoAccesso);*/
						// Se è una cartella alloco un FirstDirectoryBlock, altrimenti un FirstFileBlock
						blockRead = (inodeRead->tipoFile == 'd' ? (void*)malloc(sizeof(FirstDirectoryBlock)) : (void*)malloc(sizeof(FirstFileBlock)));
						if ( DiskDriver_readBlock(d->sfs->disk, blockRead ,inodeRead->primoBlocco) != -1 )
						{
							fprintf(stderr, "SimpleFS_readDir -> letto dal disco il primo blocco indicato dall'inode\n");
							if ( inodeRead->tipoFile == 'd' )
							{
								//fprintf(stderr, "SimpleFS_readDir -> letto dal disco l'inode\n");
								fprintf(stderr, "SimpleFS_readDir() -> %s in posizione %d\n", ((FirstDirectoryBlock*)blockRead)->fcb.name, indexNames);
								strncpy(names[indexNames], ((FirstDirectoryBlock*)blockRead)->fcb.name, 128);
							}
							else
							{
								//fprintf(stderr, "SimpleFS_readDir -> letto dal disco l'inode\n");
								fprintf(stderr, "SimpleFS_readDir() -> %s in posizione %d\n", ((FirstFileBlock*)blockRead)->fcb.name, indexNames);
								strncpy(names[indexNames], ((FirstFileBlock*)blockRead)->fcb.name, 128);
							}
							indexNames = indexNames + 1;
						}
						else
						{
							fprintf(stderr, "SimpleFS_readDir() -> errore durante la lettura del primo blocco del file/cartella\n");
						}
						free(blockRead);						
					}
					else
					{
						fprintf(stderr, "SimpleFS_readDir() -> Errore durante la lettura dell'inode\n");
					}
					free(inodeRead);
				}
				else
				{
					//fprintf(stderr, "SimpleFS_readDir() -> Inode non valido\n");
				}
				indexFiles = indexFiles + 1;
			}
		}
		// Altrimenti è il Directory Block quindi avrò al massimo (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int) files
		else
		{
			fprintf(stderr, "SimpleFS_readDir() -> sono nel DirectoryBlock\n");
			dimension = (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int);
			indexFiles = 0;
			// Ho già il DirectoryBlock allocato in directory
			while ( indexFiles < dimension )
			{
				// Se effettivamente è presente un indice inode valido relativo al file...
				if ( directory->file_inodes[indexFiles] != -1 )
				{
					fprintf(stderr, "SimpleFS_readDir() -> Nel DirectoryBlock in posizione %d c'è un inode valido\n", indexFiles);
					// ... Ottengo l'inode e successivamente il FirstFileBlock/FirstDirectoryBlock indicato dall'inode acquisito
					// e da esso leggo il nome del file contenuto nel FileControlBlock
					inodeRead = (Inode*)malloc(sizeof(Inode));
					//if ( DiskDriver_readInode(d->sfs->disk, inodeRead ,d->fdb->file_inodes[indexFiles]) != -1 )
					if ( DiskDriver_readInode(d->sfs->disk, inodeRead, directory->file_inodes[indexFiles]) != -1 )
					{
						fprintf(stderr, "SimpleFS_readDir() -> Nel DirectoryBlock letto l'inode %d\n", directory->file_inodes[indexFiles]);
						// Se è una cartella alloco un FirstDirectoryBlock, altrimenti un FirstFileBlock
						blockRead = (inodeRead->tipoFile == 'd' ? malloc(sizeof(FirstDirectoryBlock)) : malloc(sizeof(FirstFileBlock)));
						if ( DiskDriver_readBlock(d->sfs->disk, blockRead ,inodeRead->primoBlocco) != -1 )
						{
							fprintf(stderr, "SimpleFS_readDir() -> Nel DirectoryBlock letto il primo blocco %d\n", inodeRead->primoBlocco);
							fprintf(stderr, "SimpleFS_readDir() -> Leggo il nome file numero %d\n", indexNames);
							if ( inodeRead->tipoFile == 'd' )
							{
								fprintf(stderr, "SimpleFS_readDir() -> nome cartella = %s da mettere nella posizione %d\n", ((FirstDirectoryBlock*)blockRead)->fcb.name, indexNames);
								strncpy(names[indexNames], ((FirstDirectoryBlock*)blockRead)->fcb.name, 128);
								fprintf(stderr, "SimpleFS_readDir() -> Nel DirectoryBlock letto il nome di una cartella\n");
							}
							else
							{
								fprintf(stderr, "SimpleFS_readDir() -> nome file = %s da mettere nella posizione %d\n", ((FirstFileBlock*)blockRead)->fcb.name, indexNames);
								strncpy(names[indexNames], ((FirstFileBlock*)blockRead)->fcb.name, 128);
								fprintf(stderr, "SimpleFS_readDir() -> Nel DirectoryBlock letto il nome di una cartella\n");
							}
							indexNames = indexNames + 1;
						}
						free(blockRead);						
					}
					free(inodeRead);
				}
				indexFiles = indexFiles + 1;
			}
			
		}
		// Passo al blocco successivo
		//indexBloccoAttuale = d->fdb->header->next_block;
		indexBloccoAttuale = blockHeader->next_block;
		// Se esiste il blocco...
		if ( indexBloccoAttuale != -1 )
		{
			// ... lo leggo
			fprintf(stderr, "SimpleFS_readDir() -> Prossimo blocco = %d\n", indexBloccoAttuale);
			directory = (DirectoryBlock*) malloc(sizeof(DirectoryBlock));
			if ( DiskDriver_readBlock(d->sfs->disk, directory, indexBloccoAttuale) != -1 )
			{
				fprintf(stderr, "SimpleFS_readDir() -> Primo indice inode all'interno del nuovo blocco = %d\n", directory->file_inodes[0]);
				// Estraggo il Blockheader e lo memorizzo nella DirectoryHandle (mi conviene?)
				free(blockHeader);
				blockHeader = (BlockHeader*)malloc(sizeof(BlockHeader));
				memcpy(blockHeader, &(directory->header), sizeof(BlockHeader));
			}
		}
		else
		{
			// Altrimenti non esiste alcun blocco successivo 
			blockHeader = NULL;
		}
		//indexNames = indexNames + 1;
	}
	return 0;
}


// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename){
	
	int i,bound,in_index,ret;
	
	FileHandle* f=NULL;
	Inode* inode;
	FirstFileBlock* ffb;
	if(d==NULL || d->fdb==NULL)
		return NULL;
	
	//controllo pre-esistenza del file filename
	bound=BLOCK_SIZE-sizeof(BLOCK_SIZE
		   -sizeof(BlockHeader)
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
	unsigned char found=FALSE; 
	for(i=0;i<bound;i++){
		in_index=d->fdb->file_inodes[i];
		inode=malloc(sizeof(Inode));
		ret=DiskDriver_readInode(d->sfs->disk,inode,in_index);
		if(ret<0) return NULL;
		ffb=malloc(sizeof(FirstFileBlock));
		ret=DiskDriver_readBlock(d->sfs->disk,ffb,inode->primoBlocco);
		if(ret<0) return NULL;
		if( strcmp(ffb->fcb.name,filename)==0) found=TRUE;	//confronto i nomi
		}
	if(!found) return NULL;
	
	DiskDriver_init(f->sfs->disk,filename,inode->dimensioneInBlocchi);
	
	f->inode=in_index;
	f->ffb=ffb;
	f->current_block=&(ffb->header);
	f->pos_in_file=0;
	
	
	
	return f;
	
	}


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
	
	if(f==NULL || f->ffb ==NULL){
		perror("not valid file");
		return -1;
		}
	
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
			num_blocks_to_write= ceil( (double)( size-(BLOCK_SIZE - sizeof(FileControlBlock)-sizeof(BlockHeader) ) )
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
	
	if(f==NULL || f->ffb ==NULL){
		perror("not valid file");
		return -1;
		}	
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
			num_blocks_to_read= ceil((double) ( size-(BLOCK_SIZE - sizeof(FileControlBlock)-sizeof(BlockHeader) ) )
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
int SimpleFS_seek(FileHandle* f, int pos){
	/*
	 pos in file, ex:    dove [...]=blocco di 512 B , H1=header, H2=fcb, ***= data
	file:	{ [H1|H2|***] [H1|***] [H1|***] [H1|***]  ...}
												 ^pos
	*/						
							 
	int ret=0,bytes_read=0;
	if(f==NULL || f->ffb ==NULL){
		perror("not valid file");
		return -1;
		}
	
	if(pos < 0){
		perror("negative position");
		return -1;
		}

	
	//Mi prendo l' inode per sapere la dimensione del file e altre info metadata
	Inode* inode=malloc(sizeof(Inode));
	ret=DiskDriver_readInode(f->sfs->disk,inode,f->inode);					 
	if(ret < 0) return -1;
	
	if( pos > inode->dimensioneFile ){
		perror("file too short");
		return -1;
		}
		
	FileBlock* fbr=malloc(sizeof(FileBlock));
	FirstFileBlock* ffbr=malloc(sizeof(FirstFileBlock));
		
	//In quale blocco è pos?
	int num_block,payload1_bytes= BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock);	//payload in ffb
	int payload2_bytes= BLOCK_SIZE-sizeof(BlockHeader);
	
	if(pos < payload1_bytes){
		num_block=1;	// è nel ffb
	}
	else if (pos== payload1_bytes){
		num_block=2;	//è nel secondo blocco
	}
	else{
		num_block= ceil( (pos - payload1_bytes)/payload2_bytes );	//è in uno dei successivi blocchi
	}
	
	if(num_block==1){
		ret=DiskDriver_readBlock(f->sfs->disk,ffbr,num_block);
		if(ret <0) return -1;
		f->current_block= &(ffbr->header);
	}
	else{
		ret=DiskDriver_readBlock(f->sfs->disk,fbr,num_block);
		if(ret <0) return -1;
		f->current_block= &(fbr->header);
	}
	
	f->pos_in_file=pos;    
	bytes_read=pos;	//correct ?
	
	inode->dataUltimoAccesso=time(NULL);
	if(DiskDriver_writeInode(f->sfs->disk,inode,f->inode) < 0){
		perror("updating inode failed");
		return -1;
	}
	
	free(ffbr);
	free(fbr);
	free(inode);
	
	return bytes_read;
	}

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
			indexData= d->sfs->disk->header->dataFirst_free_block;

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
			
			firstDirectoryBlock->num_entries=0;

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
/*
 * 		file == foglia, eliminazione ricorsiva
 * 
 * 		
 * 					[/] (root)
 * 					/     \
				   /       \
 * 	{[f1] [/a1/b1] [f2]}    {[f1] [/a2/b1] [f2]}
 * 							        /
 * 							       /
 * 					{[f1] [/a2/b1/c1] [f2]}
 * 									/
 * 									...
 * 
 * Se chiamo remove su current dir. /a2 devo eliminare ric. tutti i suoi figli (dir comprese)
 * 
 * 
*/
//remove the file specified with its blocks
//returns -1 error, 0 on success
static int single_remove(SimpleFS* fs,FirstFileBlock* ffb,Inode* inode){
	int ret;
	ret=DiskDriver_freeBlock(fs->disk,inode->primoBlocco);
	if(ret<0) return -1;
	FileBlock* fb;
	int next_b=ffb->header.next_block;
	while(next_b != -1){ //se il file ha piu di un blocco ,li cancello tutti
			fb=malloc(sizeof(FileBlock));
			//libero il next_b-esimo blocco del file
			ret=DiskDriver_freeBlock(fs->disk,next_b);
			if(ret<0) return -1;
			//passo al prox
			next_b=fb->header.next_block;
			free(fb);
			
		}
	return 0;
}

//removes all the childs of fdbr (both r files and d files)
//returns 0 on success, -1 otherwise
static int rec_remove(SimpleFS* fs,FirstDirectoryBlock* fdbr,Inode* inode,int num_removed){
	int ret;
	//num_removed= r_remove(fs,fdbr->file_inodes);
	//if(num_removed < 0) return -1;
	int limit= (BLOCK_SIZE
		   -sizeof(BlockHeader)
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
	
	if(fdbr==NULL)
		return num_removed;
	Inode* in;
	DirectoryBlock* dbr=malloc(sizeof(DirectoryBlock));
	FirstFileBlock* ffb;
	int i,j,n_block=0;
	for(j=0;n_block>=0;j++){
		
		for(i=0;i<limit;i++){
			if(fdbr->file_inodes[i] < 0)
				break;
			in=malloc(sizeof(Inode));
			ret=DiskDriver_readInode(fs->disk,in,fdbr->file_inodes[i]);
			if(ret<0) return -1;
			if(in->tipoFile=='r'){	//è un file e va eliminato
				ffb=malloc(sizeof(FirstFileBlock));
				ret=DiskDriver_readBlock(fs->disk,ffb,in->primoBlocco);
				if(ret<0) return -1;
				ret=single_remove(fs,ffb,in);
				free(ffb);
				if(ret<0) return -1;
				num_removed++;
			}
			else if (in->tipoFile=='d'){	//dfs ricorsiva per cancellazione sotto cartelle e sottofile
				ret=DiskDriver_readBlock(fs->disk,fdbr,in->primoBlocco);
				rec_remove(fs,fdbr,in,num_removed);
				ret=DiskDriver_freeBlock(fs->disk,in->primoBlocco);
				if(ret<0) return -1;
				}
			else
				return -1;
			
			free(in);
		}
		if(j==0){
			n_block=fdbr->header.next_block;
			}
		else
		{
			ret=DiskDriver_readBlock(fs->disk,dbr,n_block);	
			if(ret<0) return -1;
			n_block=dbr->header.next_block;
		}	
	}
	free(dbr);
	return num_removed;
	}

// Funzione ausiliare per inserire gli inode nelle directory ( ricercando un -1 da poter rimpiazzare )
// Se un blocco è pieno si necessita un nuovo blocco DirectoryBlock che si crea
// In caso di errore restituisce -1, altrimenti il valore dell'inode passato come parametro (qualcosa)
int SimpleFS_insertInodeInDirectory(DirectoryHandle* d, int inode)
{
	// DirectoryBlock inizializzata nel caso si andasse dopo il FirstDirectoryBlock
	DirectoryBlock* directoryBlock = NULL;
	DirectoryBlock* precedenteDirectoryBlock = NULL;
	DirectoryBlock* nuovoDirectoryBlock = NULL;
	int indiceBloccoAttuale = d->fdb->fcb.block_in_disk;
	int indiceBloccoPrecedente = -1;
	int indiceInBloccoPrecedente; // Blocco precedente se c'è fcb = 0, o altri casi..
	int index = 0;
	int ret = -1;
	int dimension;
	int indiceBloccoNuovo;
	int i;
	// Verifica che ci sia spazio nel FirstDirectoryBlock per inserire l'inode
	dimension = (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)-sizeof(int))/sizeof(int);
	while ( index < dimension && ret == -1)
	{
		if ( d->fdb->file_inodes[index] == -1 )
		{
			ret = inode;
			d->fdb->file_inodes[index] = inode;
			// Scrivo la modifica su disco
			if ( DiskDriver_writeBlock(d->sfs->disk, d->fdb, d->fdb->fcb.block_in_disk) != -1 )
			{
				// OK 
				ret = inode;
			}
			else
			{
				fprintf(stderr, "SimpleFS_insertInodeInDirectory() -> Errore durante la scrittura/modifica di un FirstDirectoryBlock a indice %d\n", d->fdb->fcb.block_in_disk);
			}
		}
		index = index + 1;
	}
	// Se ancora non è stato inserito, verifico nei blocchi successivi se esistono
	if ( ret == -1 )
	{
		indiceBloccoPrecedente = indiceBloccoAttuale;
		indiceInBloccoPrecedente = d->fdb->header.block_in_file;
		indiceBloccoAttuale = d->fdb->header.next_block;
		while ( indiceBloccoAttuale != -1 && ret == -1 )
		{
			directoryBlock = (DirectoryBlock*) malloc(sizeof(DirectoryBlock));
			// Leggo dal disco il blocco DirectoryBlock
			if ( DiskDriver_readBlock(d->sfs->disk, directoryBlock, indiceBloccoAttuale) != -1 )
			{
				// Verifica che ci sia spazio nel DirectoryBlock per inserire l'inode
				index = 0;
				dimension = (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int);
				while ( index < dimension && ret == -1)
				{
					if ( directoryBlock->file_inodes[index] == -1 )
					{
						ret = inode;
						directoryBlock->file_inodes[index] = inode;
						// Scrivo la modifica su disco
						if ( DiskDriver_writeBlock(d->sfs->disk, directoryBlock, indiceBloccoAttuale) != -1 )
						{
							// OK 
							ret = inode;
						}
						else
						{
							fprintf(stderr, "SimpleFS_insertInodeInDirectory() -> Errore durante la scrittura/modifica di un DirectoryBlock a indice %d\n", indiceBloccoAttuale);
						}
					}
					index = index + 1;
				}
			}
			else
			{
				fprintf(stderr, "SimpleFS_insertInodeInDirectory() -> Errore durante la lettura di un DirectoryBlock a indice %d\n", indiceBloccoAttuale);
			}
			
			// Se ancora non è stato trovato spazio per l'inode, passo al blocco successivo
			if ( ret == -1 )
			{
				indiceBloccoPrecedente = indiceBloccoAttuale;
				indiceInBloccoPrecedente = directoryBlock->header.block_in_file;
				indiceBloccoAttuale = directoryBlock->header.next_block;
				if ( precedenteDirectoryBlock != NULL) 
				{
					free(precedenteDirectoryBlock);
				}
				precedenteDirectoryBlock = directoryBlock;
			}
		}
		
		// Se anche dopo questa iterazione ancora non è stato inserito, creo un nuovo blocco
		if ( ret == -1 )
		{
			fprintf(stderr, "SimpleFS_insertInodeInDirectory() -> Creazione nuovo DirectoryBlock, tutti i precedenti erano occupati\n");
			nuovoDirectoryBlock = (DirectoryBlock*) malloc(sizeof(DirectoryBlock));
			nuovoDirectoryBlock->header.previous_block = indiceBloccoPrecedente;
			nuovoDirectoryBlock->header.next_block = -1;
			nuovoDirectoryBlock->header.block_in_file = indiceInBloccoPrecedente+1;
			nuovoDirectoryBlock->file_inodes[0] = inode;
			// Setto il resto degli inode a -1
			i = 1;
			while ( i < ((BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int)) )
			{
				nuovoDirectoryBlock->file_inodes[i] = -1;
				i++;
			}
			// Ottengo il primo blocco libero
			indiceBloccoNuovo = DiskDriver_getFreeBlock(d->sfs->disk, 0);
			if ( indiceBloccoNuovo != -1 )
			{
				// Scrittura del nuovo blocco su disco all'indiceBloccoNuovo
				if ( DiskDriver_writeBlock(d->sfs->disk, nuovoDirectoryBlock, indiceBloccoNuovo) != -1 )
				{
					// Se il blocco numero 1 della cartella allora il precedente è il FirstDirectoryBlock nella DirectoryHandle passato come parametro
					if ( nuovoDirectoryBlock->header.block_in_file == 1 )
					{		
						// Aggiorno l'header del FirstDirectoryBlock
						d->fdb->header.next_block = indiceBloccoNuovo;
						// Scrivo su file il FirstDirectoryBlock a indice d->fdb->fcb->block_in_disk
						if ( DiskDriver_writeBlock(d->sfs->disk, d->fdb, d->fdb->fcb.block_in_disk) != -1 )
						{
							// OK
							fprintf(stderr, "SimpleFS_insertInodeInDirectory() -> Nuovo DirectoryBlock (il primo dopo il FirstDirectoryBlock creato in posizione %d\n", indiceBloccoNuovo);
							ret = inode;
						}
						else
						{
							fprintf(stderr, "SimpleFS_insertInodeInDirectory() -> Errore durante la scrittura del FirstDirectoryBlock a indice %d\n", d->fdb->fcb.block_in_disk);
						}
					}
					// Altrimenti il precedente è in precedenteDirectoryBlock
					else
					{
						// Aggiorno l'header del precendete DirectoryBlock
						precedenteDirectoryBlock->header.next_block = indiceBloccoNuovo;
						// Scrivo su file il DirectoryBlock a indice indiceBloccoPrecedente
						if ( DiskDriver_writeBlock(d->sfs->disk, precedenteDirectoryBlock, indiceBloccoPrecedente) != -1 )
						{
							// OK
							ret = inode;
						}
						else
						{
							fprintf(stderr, "SimpleFS_insertInodeInDirectory() -> Errore durante la scrittura del DirectoryBlock a indice %d\n", indiceBloccoPrecedente);
						}
					}
				}
				else
				{
					fprintf(stderr, "SimpleFS_insertInodeInDirectory() -> Errore durante la scrittura del nuovo DirectoryBlock a indice %d\n", indiceBloccoNuovo);
				}
			}
			else
			{
				fprintf(stderr, "SimpleFS_insertInodeInDirectory() -> Non ci sono blocchi data liberi\n");
				
			}
		}
	}
	return ret;
}


// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
// if a file removes simply that file
int SimpleFS_remove(SimpleFS* fs, char* filename){
	int ret;
	if (fs==NULL || fs->disk==NULL){
		perror("fs not valid, field(s) not found");
		return -1;
		}
	int in_index=fs->current_directory_inode;
	assert(in_index >= 0);
	
	Inode* inode=malloc(sizeof(Inode));
	ret= DiskDriver_readInode(fs->disk,inode,in_index);	//leggo l' inode della cur. dir.
	if(ret<0) return -1;
	
	assert(inode->tipoFile=='d');
	
	//prendo l' indice del primo blocco di tipo FirstDirectoryBlock 
	int block_index=inode->primoBlocco;

	//parto dal primo blocco (per forza FirstDirectory perche cur. dir è una dir)
	FirstDirectoryBlock* fdbr=malloc(sizeof(FirstDirectoryBlock));
	ret=DiskDriver_readBlock(fs->disk,fdbr,block_index);	// leggo il primo blocco dir. della cur dir
	if(ret<0) return -1;
	
	
	//scorro la dir. corrente per vedere se trovo il file con name filename e se è un file o una dir
	int i,j,limit;
	limit= (BLOCK_SIZE
		   -sizeof(BlockHeader)
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
	
	FirstFileBlock* ffb;
	DirectoryBlock* dbr=malloc(sizeof(DirectoryBlock));
	unsigned char found_file=FALSE;
	int n_block=0;
	for(j=0;n_block >= 0;j++){	//scorro tutti i blocchi della directory
		for(i=0;i<limit;i++){	//scorro tutti i file_inodes[] nel blocco
			if(j==0){ //è fdbr
				if(fdbr->file_inodes[i] <0 ) break;
				ret=DiskDriver_readInode(fs->disk,inode,fdbr->file_inodes[i]);	//inode overwritten
				n_block=fdbr->header.next_block;
			}
			else{ //è dbr
				if(dbr->file_inodes[i] <0 ) break;
				ret=DiskDriver_readInode(fs->disk,inode,dbr->file_inodes[i]); //inode overwritten
				n_block=dbr->header.next_block;
			}
			if(ret<0) return -1;
			if(inode->tipoFile=='r'){ //è un file
				ffb=malloc(sizeof(FirstFileBlock));
				ret=DiskDriver_readBlock(fs->disk,ffb,inode->primoBlocco);	//mi basta il primo blocco quello con fcb
				if(strcmp(ffb->fcb.name,filename)==0){	//trovato il file e basta rimuoverlo
					single_remove(fs,ffb,inode);
					found_file=TRUE;
					break;
					}
				free(ffb);
			}
			else if (inode->tipoFile=='d'){	//è una dir
				FirstDirectoryBlock* fdb= malloc(sizeof(FirstDirectoryBlock));
				ret=DiskDriver_readBlock(fs->disk,fdb,inode->primoBlocco);
				if(ret<0) return -1;
				if(strcmp(fdb->fcb.name,filename)==0){	//trovata la dir con name filename , eliminaz. ricorsiva
					rec_remove(fs,fdb,inode,0);
					found_file=TRUE;
					break;
					}
				free(fdb);
				}
			else{
				perror("not recognized tipoFile");
				return -1;
				}
				
		}
		if(found_file)
			break;
		ret=DiskDriver_readBlock(fs->disk,dbr,n_block);	//continuo la ricerca su altri blocchi (carico dbr)
	}
	if(!found_file){
		fprintf(stderr,"File to remove: %s not found",filename);
		return -1;
			}
	free(dbr);
	free(inode);
	free(fdbr);
	return 0;
	
	}


  

