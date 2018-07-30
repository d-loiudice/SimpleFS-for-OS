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
			directoryHandle->fdb = (FirstDirectoryBlock*) malloc(sizeof(FirstDirectoryBlock));
			// L'indice del blocco data contenente i dati è nell'inode
			if ( DiskDriver_readBlock(disk, (void*)directoryHandle->fdb, inode->primoBlocco) != -1 )
			{
				// Setto i restanti campi della directory handle
				directoryHandle->current_block = &(directoryHandle->fdb->header); // Sono nel primo blocco
				fs->current_directory_inode = 0;
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
	char** contenutoDirectory = (char**)malloc(d->fdb->num_entries*sizeof(char*));
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
							// Aggiorno la struttura dati della cartella in cui ho creato il file
							d->fdb->num_entries = d->fdb->num_entries + 1;
							// E la riscrivo su file
							if ( DiskDriver_writeBlock(d->sfs->disk, d->fdb, d->fdb->fcb.block_in_disk) != - 1)
							{
								// OK
								fprintf(stderr, "SimpleFS_createFile() -> Ora ci sono %d files\n", d->fdb->num_entries);
							}
							else
							{
								fprintf(stderr, "SimpleFS_createFile() -> Errore durante la sovrascrittura del FirstDirectoryBlock della cartella padre\n");
							}
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
		fprintf(stderr, "SimpleFS_createFile -> File %s già esistente in %s \n",filename,d->fdb->fcb.name);
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
	DiskDriver_flush(d->sfs->disk);
	return fileHandle;
}
//TODEL bitmaps

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
	Inode* inodeRead = NULL;
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
				//fprintf(stderr, "SimpleFS_readDir() -> Inode letto a posizione %d: %d\n",indexFiles, d->fdb->file_inodes[indexFiles]);
				// Se effettivamente è presente un indice inode valido relativo al file...
				if ( d->fdb->file_inodes[indexFiles] != -1 )
				{
					//fprintf(stderr, "SimpleFS_readDir() -> inode letto: %d\n", d->fdb->file_inodes[indexFiles]);
					//fprintf(stderr, "SimpleFS_readDir() -> indexFiles negli inode: %d\n", indexFiles);
					// ... Ottengo l'inode e successivamente il FirstFileBlock/FirstDirectoryBlock indicato dall'inode acquisito
					// e da esso leggo il nome del file contenuto nel FileControlBlock
					inodeRead = (Inode*)malloc(sizeof(Inode));
					fprintf(stderr, "SimpleFS_readDir() -> inodeRead allocato a indirizzo %p\n", inodeRead);
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
								strncpy(names[indexNames], ((FirstDirectoryBlock*)blockRead)->fcb.name, 127);
							}
							else
							{
								//fprintf(stderr, "SimpleFS_readDir -> letto dal disco l'inode\n");
								fprintf(stderr, "SimpleFS_readDir() -> %s in posizione %d\n", ((FirstFileBlock*)blockRead)->fcb.name, indexNames);
								strncpy(names[indexNames], ((FirstFileBlock*)blockRead)->fcb.name, 127);
							}
							indexNames = indexNames + 1;
						}
						else
						{
							fprintf(stderr, "SimpleFS_readDir() -> errore durante la lettura del primo blocco del file/cartella\n");
						}
						//fprintf(stderr, "SimpleFS_readDir() -> free blockread?\n");
						free(blockRead);					
						//fprintf(stderr, "SimpleFS_readDir() -> free blockread! OK\n");	
					}
					else
					{
						fprintf(stderr, "SimpleFS_readDir() -> Errore durante la lettura dell'inode\n");
					}
					//fprintf(stderr, "SimpleFS_readDir() -> free inoderead?\n");
					//fprintf(stderr, "%d\n", inodeRead->primoBlocco);
					//fprintf(stderr, "%c\n", inodeRead->tipoFile);
					//fprintf(stderr, "SimpleFS_readDir() -> inodeRead dealloco a indirizzo %d\n", inodeRead);
					free(inodeRead);
					//fprintf(stderr, "SimpleFS_readDir() -> free inoderead! OK\n");
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
				fprintf(stderr, "SimpleFS_readDir() -> free blockHeader?\n");
				free(blockHeader);
				fprintf(stderr, "SimpleFS_readDir() -> free blockHeader? OK\n");
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
	DiskDriver_flush(d->sfs->disk);
	return 0;
}


// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename){
	
	int i,bound,in_index,ret;
	
	FileHandle* f=NULL;
	Inode* inode;
	FirstFileBlock* ffb;
	DirectoryBlock* db;
	if(d==NULL || d->fdb==NULL)
		return NULL;
	
	//controllo pre-esistenza del file filename
	bound=BLOCK_SIZE-sizeof(BLOCK_SIZE
		   -sizeof(BlockHeader)
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
	unsigned char found=FALSE,no_db=FALSE; 
	if(d->fdb->num_entries <= bound)
		no_db=TRUE;
	
	if(no_db==TRUE){
		i=0;
		while(!found && i < d->fdb->num_entries){
			in_index=d->fdb->file_inodes[i];
			if(in_index < 0) continue;
			inode=malloc(sizeof(Inode));
			ret=DiskDriver_readInode(d->sfs->disk,inode,in_index);
			if(ret<0) return NULL;
			if(inode->tipoFile!='r'){
				fprintf(stderr,"%s -> Not valid file name (it's a directory)\n",__func__);
				return NULL;
				}
			ffb=malloc(sizeof(FirstFileBlock));
			ret=DiskDriver_readBlock(d->sfs->disk,ffb,inode->primoBlocco);
			if(ret<0) return NULL;
			if( strcmp(ffb->fcb.name,filename)==0) found=TRUE;	//confronto i nomi
			i++;
			}
	}
	else{
		db=malloc(sizeof(DirectoryBlock));
		int index_block=0;
		int num_db=(int) ceil( (double)(d->fdb->num_entries-bound) / ((BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int)) );
		int next=-1;
		while( index_block < num_db + 1){
			
			if(next > 0){	//inizialmente no
				ret=DiskDriver_readBlock(d->sfs->disk,db,next);	//lettura eventuale db successivo
				if(ret<0) return NULL;
			}
			else
				break;
			for(i=0;i < bound; i++){
				if(index_block==0)
					in_index=d->fdb->file_inodes[i];
				else
					in_index=db->file_inodes[i];
				if(in_index < 0) continue; 
				inode=malloc(sizeof(Inode));
				ret=DiskDriver_readInode(d->sfs->disk,inode,in_index);
				if(ret<0) return NULL;
				if(inode->tipoFile!='r'){
				fprintf(stderr,"%s -> Not valid file name (it's a directory)\n",__func__);
				return NULL;
				}
				ffb=malloc(sizeof(FirstFileBlock));
				ret=DiskDriver_readBlock(d->sfs->disk,ffb,inode->primoBlocco);
				if(ret<0) return NULL;
				if( strcmp(ffb->fcb.name,filename)==0) found=TRUE;	//confronto i nomi
				}
			if(index_block==0)
				next=d->fdb->header.next_block;
			else{
				next=db->header.next_block;
				}
			
		
			index_block++;
		
		}
		free(db);
	}
	if(!found) return NULL;
	
	//DiskDriver_init(f->sfs->disk,filename,inode->dimensioneInBlocchi);
	f = (FileHandle*) malloc(sizeof(FileHandle));
	f->sfs = d->sfs;
	f->inode=in_index;
	f->ffb=ffb;
	f->current_block=&(ffb->header);
	f->pos_in_file=0;
	
	Inode* inodeAggiornamento = malloc(sizeof(Inode));
	if ( DiskDriver_readInode(d->sfs->disk, inodeAggiornamento, f->inode) != -1 )
	{
		inodeAggiornamento->dataUltimoAccesso = time(NULL);
		if ( DiskDriver_writeInode(d->sfs->disk, inodeAggiornamento, f->inode) != -1 )
		{
			// OK
		}
		else
		{
			fprintf(stderr, "SimpleFS_openFile() -> errore durante la scrittura dell'aggiornamento data ultimo accesso dell'inode %d\n", f->inode);
		}
	}
	else
	{
		fprintf(stderr, "SimpleFS_openFile() -> Errore durante lettura per l'aggiornamento data ultimo accesso dell'inode %d\n", f->inode);
	}
	free(inodeAggiornamento);
	
	return f;
}


// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f){
	if (f==NULL) {
		fprintf(stderr,"SimpleFS_close -> file not valid (null)	\n");
		return -1;
	}
	if (f->current_block != &(f->ffb->header)) free(f->current_block);
	free(f->ffb);
	free(f);
	//printf("ciaone\n");
	return 1;

}

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size){
	
	if(f==NULL || f->ffb ==NULL || size <0 || data==NULL){
		perror("not valid parameter(s)");
		return -1;
		}
	
	int ret;
		
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
	
	int num_block;
	int numeroBloccoAttuale;
	int bytes_written = 0;
	int posizione = f->pos_in_file;
	int posizioneRelativa;
	int payloadFFB = BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock);
	int payloadFB = BLOCK_SIZE-sizeof(BlockHeader);
	int indiceBuffer;
	FileBlock* nuovoFileBlock = NULL;
	// In quale blocco sono
	if(posizione < payloadFFB){
		num_block=0;	// è nel ffb
	}
	else if (posizione == payloadFFB){
		num_block=1;	//è nel secondo blocco
	}
	else{
		num_block= (int)ceil( (double)(posizione - payloadFFB)/payloadFB );	//è in uno dei successivi blocchi
	}
	
	// Partendo dal FFB devo raggiungere il num_block blocco
	void* dataBloccoAttuale = f->ffb;
	int indiceBloccoAttuale = f->ffb->fcb.block_in_disk;
	int indiceBloccoSuccessivo;
	int dimensionePayloadBlocco;
	numeroBloccoAttuale = 0;
	// Se devo raggiungere il FFB già ce l'ho
	while ( numeroBloccoAttuale != num_block )
	{
		indiceBloccoSuccessivo = (numeroBloccoAttuale == 0 ? ((FirstFileBlock*)dataBloccoAttuale)->header.next_block : ((FileBlock*)dataBloccoAttuale)->header.next_block );
		// Se ho memorizzato il FFB non lo devo liberare
		if ( numeroBloccoAttuale > 0 ) free(dataBloccoAttuale);
		dataBloccoAttuale = malloc(sizeof(FileBlock));
		if ( DiskDriver_readBlock(f->sfs->disk, dataBloccoAttuale, indiceBloccoSuccessivo) != -1 )
		{
			indiceBloccoAttuale = indiceBloccoSuccessivo;
			numeroBloccoAttuale = numeroBloccoAttuale + 1;
		}
		else
		{
			fprintf(stderr, "SimpleFS_write() -> Errore durante la lettura del blocco data successivo %d\n", indiceBloccoSuccessivo);
		}
	}
	
	indiceBuffer = 0;
	// Continuo a scrivere finché non ho scritto tutto il buffer oppure è capitato è un errore (ret != 0)
	while ( bytes_written != size && ret == 0 )
	{
		// Calcolo la posizione relativa nel blocco attuale da cui iniziare a scrivere
		if ( numeroBloccoAttuale == 0)
		{
			posizioneRelativa = posizione;
		}
		else
		{
			posizioneRelativa = posizione - payloadFFB - ((numeroBloccoAttuale-1)*payloadFB);
		}
		
		// Scrittura fino alla fine del buffer oppure fino alla fine del blocco attuale
		//indiceBuffer = 0;
		dimensionePayloadBlocco = (numeroBloccoAttuale == 0 ? payloadFFB : payloadFB);
		while ( indiceBuffer < size && posizioneRelativa < dimensionePayloadBlocco )
		{
			if ( numeroBloccoAttuale == 0 )
			{
				(((FirstFileBlock*)dataBloccoAttuale)->data)[posizioneRelativa] = ((char*)data)[indiceBuffer];
			}
			else
			{
				(((FileBlock*)dataBloccoAttuale)->data)[posizioneRelativa] = ((char*)data)[indiceBuffer];
			}
			//fprintf(stderr, "SimpleFS_write() -> devo scrivere %c da posizione %d\n", ((char*)data)[indiceBuffer], indiceBuffer);
			//fprintf(stderr, "SimpleFS_write() -> se è ffb scritto %c in posizione %d\n", (((FirstFileBlock*)dataBloccoAttuale)->data)[posizioneRelativa], posizioneRelativa);
			//fprintf(stderr, "SimpleFS_write() -> se è fb scritto %c in posizione %d\n", (((FileBlock*)dataBloccoAttuale)->data)[posizioneRelativa], posizioneRelativa);
			
			posizioneRelativa = posizioneRelativa + 1;
			indiceBuffer = indiceBuffer + 1;
			posizione = posizione + 1;
			bytes_written = bytes_written + 1;
		}
		
		// Salvo le modifiche effettuate
		if ( DiskDriver_writeBlock(f->sfs->disk, dataBloccoAttuale, indiceBloccoAttuale) != -1 )
		{
			// OK
		}
		else
		{
			fprintf(stderr, "SimpleFS_write() -> errore durante la scrittura del blocco data %d\n", indiceBloccoAttuale);
		}
		
		// Verifica perché è uscito dall'iterazione
		if ( indiceBuffer < size && posizioneRelativa >= dimensionePayloadBlocco )
		{
			fprintf(stderr, "SimpleFS_write() -> è finito il blocco ma devo ancora scrivere\n");
			// Verifica se esiste un blocco successivo
			indiceBloccoSuccessivo = (numeroBloccoAttuale == 0) ? ((FirstFileBlock*)dataBloccoAttuale)->header.next_block : ((FileBlock*)dataBloccoAttuale)->header.next_block;
			if ( indiceBloccoSuccessivo != -1 )
			{
				// Esiste e lo leggo 
				indiceBloccoAttuale = indiceBloccoSuccessivo;
				if ( DiskDriver_readBlock(f->sfs->disk, dataBloccoAttuale, indiceBloccoAttuale) != -1 )
				{
					// OK
					numeroBloccoAttuale = numeroBloccoAttuale + 1;
				}
				else
				{
					fprintf(stderr, "SimpleFS_write() -> errore durante la lettura del già esistente blocco successivo %d\n", indiceBloccoAttuale);
					ret = -1;
				}
			}
			else
			{
				// Non esiste devo crearlo e allegarlo
				indiceBloccoSuccessivo = DiskDriver_getFreeBlock(f->sfs->disk, 0);
				fprintf(stderr, "Il nuovo blocco sarà in %d\n", indiceBloccoSuccessivo);
				nuovoFileBlock = (FileBlock*) malloc(sizeof(FileBlock));
				nuovoFileBlock->header.block_in_file = numeroBloccoAttuale + 1;
				nuovoFileBlock->header.previous_block = indiceBloccoAttuale;
				nuovoFileBlock->header.next_block = -1;
				if ( numeroBloccoAttuale == 0 )
				{
					((FirstFileBlock*)dataBloccoAttuale)->header.next_block = indiceBloccoSuccessivo;
				}
				else
				{
					((FileBlock*)dataBloccoAttuale)->header.next_block = indiceBloccoSuccessivo;
				}
				// Scrittura del nuovo blocco e sucessivamente riscrittura del blocco attuale per le modifiche
				if ( DiskDriver_writeBlock(f->sfs->disk, nuovoFileBlock, indiceBloccoSuccessivo) != -1 )
				{
					if ( DiskDriver_writeBlock(f->sfs->disk, dataBloccoAttuale, indiceBloccoAttuale) != -1 )
					{
						// OK 
						indiceBloccoAttuale = indiceBloccoSuccessivo;
						if ( numeroBloccoAttuale != 0 )
						{
							// Liberare se non è FDB
							free(dataBloccoAttuale);
						}
						numeroBloccoAttuale = nuovoFileBlock->header.block_in_file;
						dataBloccoAttuale = nuovoFileBlock;
						posizioneRelativa = 0;
					}
					else
					{
						fprintf(stderr, "SimpleFS_write() -> Errore durante la modifica del blocco attuale %d per inserire il nuovo blocco\n", indiceBloccoAttuale);
						ret = -1;
					}
				}
				else
				{
					fprintf(stderr, "SimpleFS_write() -> Errore durante la creazione di un nuovo blocco %d\n", indiceBloccoSuccessivo);
					ret = -1;
				}
			}
		}
		
		if ( indiceBuffer >= size )
		{
			fprintf(stderr, "SimpleFS_write() -> ho finito di scrivere il buffer good\n");
		}
	}
	//aggiornamento inode
	f->pos_in_file = posizione;
	in_read->dataUltimaModifica=time(NULL);
	fprintf(stderr, "SimpleFS_write() -> Aggiornata ultima modifica a: %ld\n", in_read->dataUltimaModifica);
	if (posizione > in_read->dimensioneFile)
	{
		in_read->dimensioneFile = posizione;
	}
	ret= DiskDriver_writeInode(f->sfs->disk,in_read,f->inode);
	if(ret < 0){
		perror("erorre write inode in write simplefs");
		return -1;
	}
	else
	{
		fprintf(stderr, "SimpleFS_write()-> Aggiornato inode per ultima modifica: %d\n", f->inode);
	}
		
	free(in_read);
	//free(ffbw);
	//free(fbw); 
	if ( nuovoFileBlock != NULL ) free(nuovoFileBlock);
	
	DiskDriver_flush(f->sfs->disk);
	
	return bytes_written;
}






// read in the file, at current position size bytes and store them in data
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size){
	if(f==NULL || f->ffb ==NULL || size <0 || data==NULL){
		perror("not valid parameter(s)");
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
	
	
	int ret;
	int num_block;	//numero di fb
	int numeroBloccoAttuale;
	int bytes_read = 0;
	int posizione = f->pos_in_file;
	int posizioneRelativa;
	int payloadFFB = BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock);
	int payloadFB = BLOCK_SIZE-sizeof(BlockHeader);
	int indiceBuffer;
	FileBlock* nuovoFileBlock = NULL;
	// In quale blocco sono
	if(posizione < payloadFFB){
		num_block=0;	// è nel ffb
	}
	else if (posizione == payloadFFB){
		num_block=1;	//è nel secondo blocco
	}
	else{
		num_block= (int)ceil( (double)(posizione - payloadFFB)/payloadFB );	//è in uno dei successivi blocchi
	}
	
	// Partendo dal FFB devo raggiungere il num_block blocco
	void* dataBloccoAttuale = f->ffb;
	int indiceBloccoAttuale = f->ffb->fcb.block_in_disk;
	int indiceBloccoSuccessivo;
	int dimensionePayloadBlocco;
	numeroBloccoAttuale = 0;
	// Se devo raggiungere il FFB già ce l'ho
	while ( numeroBloccoAttuale != num_block )
	{
		indiceBloccoSuccessivo = (numeroBloccoAttuale == 0 ? ((FirstFileBlock*)dataBloccoAttuale)->header.next_block : ((FileBlock*)dataBloccoAttuale)->header.next_block );
		// Se ho memorizzato il FFB non lo devo liberare
		if ( numeroBloccoAttuale > 0 ) free(dataBloccoAttuale);
		dataBloccoAttuale = malloc(sizeof(FileBlock));
		if ( DiskDriver_readBlock(f->sfs->disk, dataBloccoAttuale, indiceBloccoSuccessivo) != -1 )
		{
			indiceBloccoAttuale = indiceBloccoSuccessivo;
			numeroBloccoAttuale = numeroBloccoAttuale + 1;
		}
		else
		{
			fprintf(stderr, "SimpleFS_read() -> Errore durante la lettura del blocco data successivo %d\n", indiceBloccoSuccessivo);
		}
	}
	
	indiceBuffer = 0;
	// Continuo a leggere finché non ho letto tutto il buffer (size bytes) oppure è capitato è un errore (ret != 0)
	while ( bytes_read != size && ret == 0 )
	{
		// Calcolo la posizione relativa nel blocco attuale da cui iniziare a leggere
		if ( numeroBloccoAttuale == 0)
		{
			posizioneRelativa = posizione;
		}
		else
		{
			posizioneRelativa = posizione - payloadFFB - ((numeroBloccoAttuale-1)*payloadFB);
		}
		
		// Lettura fino alla fine del buffer oppure fino alla fine del blocco attuale
		//indiceBuffer = 0;
		dimensionePayloadBlocco = (numeroBloccoAttuale == 0 ? payloadFFB : payloadFB);
		while ( indiceBuffer < size && posizioneRelativa < dimensionePayloadBlocco )
		{
			if ( numeroBloccoAttuale == 0 )
			{
				((char*)data)[indiceBuffer]=(((FirstFileBlock*)dataBloccoAttuale)->data)[posizioneRelativa] ;
			}
			else
			{
				((char*)data)[indiceBuffer]=(((FileBlock*)dataBloccoAttuale)->data)[posizioneRelativa];
			}
			//fprintf(stderr, "SimpleFS_write() -> devo scrivere %c da posizione %d\n", ((char*)data)[indiceBuffer], indiceBuffer);
			//fprintf(stderr, "SimpleFS_write() -> se è ffb scritto %c in posizione %d\n", (((FirstFileBlock*)dataBloccoAttuale)->data)[posizioneRelativa], posizioneRelativa);
			//fprintf(stderr, "SimpleFS_write() -> se è fb scritto %c in posizione %d\n", (((FileBlock*)dataBloccoAttuale)->data)[posizioneRelativa], posizioneRelativa);
			
			posizioneRelativa = posizioneRelativa + 1;
			indiceBuffer = indiceBuffer + 1;
			posizione = posizione + 1;
			bytes_read = bytes_read + 1;
		}
		
	
		// Verifica perché è uscito dall'iterazione
		if ( indiceBuffer < size && posizioneRelativa >= dimensionePayloadBlocco )
		{
			fprintf(stderr, "SimpleFS_read() -> è finito il blocco ma devo ancora leggere\n");
			// Verifica se esiste un blocco successivo
			indiceBloccoSuccessivo = (numeroBloccoAttuale == 0) ? ((FirstFileBlock*)dataBloccoAttuale)->header.next_block : ((FileBlock*)dataBloccoAttuale)->header.next_block;
			if ( indiceBloccoSuccessivo != -1 )
			{
				// Esiste e lo leggo 
				indiceBloccoAttuale = indiceBloccoSuccessivo;
				if ( DiskDriver_readBlock(f->sfs->disk, dataBloccoAttuale, indiceBloccoAttuale) != -1 )
				{
					// OK
					numeroBloccoAttuale = numeroBloccoAttuale + 1;
				}
				else
				{
					fprintf(stderr, "SimpleFS_read() -> errore durante la lettura del già esistente blocco successivo %d\n", indiceBloccoAttuale);
					ret = -1;
				}
			}
			
		}
		else{
			fprintf(stderr,"SimpleFS_read() -> posizione specificata non valida \n");
			
			}
		
		if ( indiceBuffer >= size )
		{
			fprintf(stderr, "SimpleFS_read() -> ho finito di leggere il buffer good\n");
		}
	}
	//aggiornamento inode
	f->pos_in_file = posizione;
	//in_read->dataUltimaModifica=time(NULL);
	if (posizione > in_read->dimensioneFile)
	{
		//in_read->dimensioneFile = posizione;
	}
	in_read->dataUltimoAccesso=time(NULL);
	ret= DiskDriver_writeInode(f->sfs->disk,in_read,f->inode);
	if(ret < 0){
		perror("erorre write inode in read simplefs");
		return -1;
	}
		
	free(in_read);
	//free(ffbw);
	//free(fbw); 
	if ( nuovoFileBlock != NULL ) free(nuovoFileBlock);
	
	DiskDriver_flush(f->sfs->disk);
	
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
		
	//FileBlock* fbr=malloc(sizeof(FileBlock));
	FirstFileBlock* ffbr=malloc(sizeof(FirstFileBlock));
		
	//In quale blocco è pos?
	int num_block;
	int payload1_bytes= BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock);	//payload in ffb
	int payload2_bytes= BLOCK_SIZE-sizeof(BlockHeader);
	
	if(pos < payload1_bytes){
		num_block=0;	// è nel ffb
	}
	else if (pos== payload1_bytes){
		num_block=1;	//è nel secondo blocco
	}
	else{
		num_block= (int)ceil( (double)(pos - payload1_bytes)/payload2_bytes );	//è in uno dei successivi blocchi
	}
	
	FileBlock* fileBlock = malloc(sizeof(FileBlock));
	
	// Se non devo andare al FFB
	if ( num_block != 0 )
	{
		// Iterazione per raggiungere il blocco da leggere
		while ( f->current_block->block_in_file != num_block && bytes_read != -1 )
		{
			fprintf(stderr, "SimpleFS_seek() -> block_in_file = %d -- num_block = %d\n", f->current_block->block_in_file, num_block);
			if ( f->current_block->block_in_file < num_block )
			{
				fprintf(stderr, "SimpleFS_seek() -> Devo andare avanti\n");
				// Devo andare avanti
				if ( f->current_block->next_block != -1 )
				{
					fprintf(stderr, "SimpleFS_seek() -> lettura blocco data %d\n", f->current_block->next_block);
					if ( DiskDriver_readBlock(f->sfs->disk, fileBlock, f->current_block->next_block) != -1)
					{
						// Se non sono già nel ffb, devo liberare la memoria
						if ( f->current_block != &(f->ffb->header) ) 
						{
							free(f->current_block);
						}
						f->current_block = malloc(sizeof(BlockHeader));
						//memcpy(f->current_block, fileBlock->header, sizeof(BlockHeader));
						f->current_block->block_in_file = fileBlock->header.block_in_file;
						f->current_block->next_block = fileBlock->header.next_block;
						f->current_block->previous_block = fileBlock->header.previous_block;
					}
					else
					{
						fprintf(stderr, "SimpleFS_seek() -> Errore durante la lettura del successivo blocco %d\n", f->current_block->next_block);
					}
				}
				else
				{
					// Fuori file
					bytes_read  = -1;
				}
				
			}
			else
			{
				fprintf(stderr, "SimpleFS_seek() -> Devo andare indietro\n");
				// Devo andare indietro
				if ( f->current_block->previous_block != -1 )
				{
					fprintf(stderr, "SimpleFS_seek() -> lettura blocco data %d\n", f->current_block->previous_block);
					if ( DiskDriver_readBlock(f->sfs->disk, fileBlock, f->current_block->previous_block) != -1 )
					{
						free(f->current_block);
						f->current_block = malloc(sizeof(BlockHeader));
						//memcpy(f->current_block, fileBlock->header, sizeof(BlockHeader));
						f->current_block->block_in_file = fileBlock->header.block_in_file;
						f->current_block->next_block = fileBlock->header.next_block;
						f->current_block->previous_block = fileBlock->header.previous_block;
					}
					else
					{
						fprintf(stderr, "SimpleFS_seek() -> Errore durante la lettura del precedente blocco %d\n", f->current_block->previous_block);
					}
				}
				else
				{
					bytes_read = -1;
				}
			}
		}
	}
	else
	{
		// Se non ci sono già al blocco 0 
		if ( f->current_block != &(f->ffb->header) )
		{
			free(f->current_block);
			f->current_block = &(f->ffb->header);
		}
	}
	
	 
	
	/*if(num_block==1){
		ret=DiskDriver_readBlock(f->sfs->disk,ffbr,num_block);
		if(ret <0) return -1;
		f->current_block= &(ffbr->header);
	}
	else{
		ret=DiskDriver_readBlock(f->sfs->disk,fbr,num_block);
		if(ret <0) return -1;
		f->current_block= &(fbr->header);
	}*/
	
	if ( bytes_read != -1)
	{
		// Spostamento assoluto
		bytes_read = f->pos_in_file - pos;
		if ( bytes_read < 0 ) bytes_read = bytes_read*-1;
		f->pos_in_file=pos;    
		//bytes_read=pos;	//correct ?
		
		inode->dataUltimoAccesso=time(NULL);
		if(DiskDriver_writeInode(f->sfs->disk,inode,f->inode) < 0){
			perror("updating inode failed");
			return -1;
		}
	}
	else
	{
		//fprintf(stderr, "SimpleFS_seek() -> Errore durante lo spostamento 
	}
	//free(ffbr);
	free(fileBlock);
	free(inode);
	
	return bytes_read;
}

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle

int SimpleFS_changeDir(DirectoryHandle* d, char* dirname)
{
	int ret = -1;
	int i;
	//int trovato = -1;
	BlockHeader* blockHeader = NULL;
	DirectoryBlock* directoryBlock = NULL;
	Inode* inodeRead = NULL;
	FirstDirectoryBlock* fdbRead;
	int dimensione;
	int indexInode = -1;
	if ( strncmp(dirname, "..", 128) == 0 )
	{
		// Vado al livello superiore
		// Verifico che la cartella in cui sono abbia un livello superiore
		if ( d->fdb->fcb.parent_inode != -1 )
		{
			inodeRead = (Inode*) malloc(sizeof(Inode));
			indexInode = d->fdb->fcb.parent_inode;
			if ( DiskDriver_readInode(d->sfs->disk, inodeRead, indexInode) != -1 )
			{
				// Leggo il primo blocco (FDB) indicato dall'inode
				fdbRead = (FirstDirectoryBlock*) malloc(sizeof(FirstDirectoryBlock));
				if ( DiskDriver_readBlock(d->sfs->disk, fdbRead, inodeRead->primoBlocco)  != -1 )
				{
					// Side-effect sulla directoryhandle d
					fprintf(stderr, "SimpleFS_changeDir() -> Modifico la directory handle\n");
					free(d->fdb);
					d->fdb = fdbRead;
					d->inode = indexInode;
					d->current_block = &(fdbRead->header);
					d->pos_in_dir = 0;
					d->pos_in_block = 0;
					d->sfs->current_directory_inode = indexInode;
					ret = 0;
					//fprintf(stderr, "SimpleFS_changeDir() -> modificato current directory inode in: %d\n", d->sfs->current_directory_inode);
				}
				else
				{
					fprintf(stderr, "SimpleFS_changeDir() -> Errore durante lettura del FirstDirectoryBlock %d indicato dall'inode del padre della cartella corrente\n", inodeRead->primoBlocco);
					free(fdbRead);
				}
				//free(fdbRead);
			}
			else
			{
				fprintf(stderr, "SimpleFS_changeDir() -> Errore durante la lettura dell'inode %d del padre della cartella corrente\n", indexInode);
			}
			free(inodeRead);
		}
		else
		{
			fprintf(stderr, "SimpleFS_changeDir() -> La cartella è toplevel, non ha padre\n");
		}
	}
	else
	{	
		// Scorro tra i file inodes della cartella
		blockHeader = (BlockHeader*)malloc(sizeof(BlockHeader));
		// Inizializzo al valore del blockheader del fdb
		memcpy(blockHeader, &(d->fdb->header), sizeof(BlockHeader));
		while ( blockHeader != NULL && ret == -1 )
		{
			// Verifica che sia un FDB oppure un DB
			if ( blockHeader->block_in_file == 0 )
			{
				// FDB
				dimensione = (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)-sizeof(int))/sizeof(int);
				i = 0;
				while ( i < dimensione && ret == -1)
				{
					// Verifica che ci sia un indice di inode valido
					if ( d->fdb->file_inodes[i] != -1 )
					{
						// Leggo l'inode indicato
						inodeRead = (Inode*) malloc(sizeof(Inode));
						indexInode = d->fdb->file_inodes[i];
						if ( DiskDriver_readInode(d->sfs->disk, inodeRead, indexInode) != -1 )
						{
							// Verifica che sia una cartella
							if ( inodeRead->tipoFile == 'd' )
							{
								// Lettura del primo blocco indicato dall'inode
								fdbRead = (FirstDirectoryBlock*) malloc(sizeof(FirstDirectoryBlock));
								if ( DiskDriver_readBlock(d->sfs->disk, fdbRead, inodeRead->primoBlocco) != -1 )
								{
									// Leggo il nome del file
									if ( strncmp(fdbRead->fcb.name, dirname, 128) == 0 )
									{
										fprintf(stderr, "SimpleFS_changeDir() -> Trovata la directory %s\n", fdbRead->fcb.name);
										// Creazione directoryHandle o modifica? -> modifica
										free(d->fdb);
										d->fdb = fdbRead;
										d->inode = indexInode;
										d->current_block = &(fdbRead->header);
										d->pos_in_dir = 0;
										d->pos_in_block = 0;
										d->sfs->current_directory_inode = indexInode;
										ret = 0;
										// OK
									}
								}
								else
								{
									fprintf(stderr, "SimpleFS_changeDir() -> Errore durante la lettura del blocco %d indicato dall'inode\n", inodeRead->primoBlocco);
								}
								// Se ancora non ho trovato, libero la memoria. Altrimenti non la devo liberare perché sarà nella directoryHandle
								if ( ret == -1 ) 
								{
									free(fdbRead);
								}
							}
						}
						else
						{
							fprintf(stderr, "SimpleFS_changeDir() -> Errore durante la lettura dell'inode %d\n", d->fdb->file_inodes[i]);
						}
						free(inodeRead);
					}
					i++;
				}
			}
			else
			{
				// DB
				dimensione = (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int);
				i = 0;
				while ( i < dimensione && ret == -1) 
				{
					// Verifica che ci sia un indice di inode valido
					if ( directoryBlock->file_inodes[i] != -1 )
					{
						// Leggo l'inode indicato
						inodeRead = (Inode*) malloc(sizeof(Inode));
						indexInode = directoryBlock->file_inodes[i];
						if ( DiskDriver_readInode(d->sfs->disk, inodeRead, indexInode) != -1 )
						{
							// Verifica che sia una cartella
							if ( inodeRead->tipoFile == 'd' )
							{
								// Lettura del primo blocco indicato dall'inode
								fdbRead = (FirstDirectoryBlock*) malloc(sizeof(FirstDirectoryBlock));
								if ( DiskDriver_readBlock(d->sfs->disk, fdbRead, inodeRead->primoBlocco) != -1 )
								{
									// Leggo il nome del file
									if ( strncmp(fdbRead->fcb.name, dirname, 128) == 0 )
									{
										fprintf(stderr, "SimpleFS_changeDir() -> Trovata la directory %s\n", fdbRead->fcb.name);
										// Creazione directoryHandle o modifica? -> modifica
										free(d->fdb);
										d->fdb = fdbRead;
										d->inode = indexInode;
										d->current_block = &(fdbRead->header);
										d->pos_in_dir = 0;
										d->pos_in_block = 0;
										ret = 0;
										// OK
									}
								}
								else
								{
									fprintf(stderr, "SimpleFS_changeDir() -> Errore durante la lettura del blocco %d indicato dall'inode\n", inodeRead->primoBlocco);
								}
								// Se ancora non ho trovato, libero la memoria. Altrimenti non la devo liberare perché sarà nella directoryHandle
								if ( ret == -1 ) 
								{
									free(fdbRead);
								}
							}
						}
						else
						{
							fprintf(stderr, "SimpleFS_changeDir() -> Errore durante la lettura dell'inode %d\n", d->fdb->file_inodes[i]);
						}
						free(inodeRead);
						inodeRead = NULL;
					}
					i++;
				}
			}
			// Passo al blocco successivo
			if ( blockHeader->next_block != -1 )
			{
				//free(blockHeader);
				directoryBlock = (DirectoryBlock*) malloc(sizeof(DirectoryBlock));
				// Leggo il blocco indicato in next_block
				if ( DiskDriver_readBlock(d->sfs->disk, directoryBlock, blockHeader->next_block) != -1 )
				{
					// Mi copio il blockHeader
					memcpy(blockHeader, &(directoryBlock->header), sizeof(BlockHeader));
				}
				else
				{
					fprintf(stderr, "SimpleFS_changeDir() -> Lettura blocco directoryBlock fallita a indice %d\n", blockHeader->next_block);
				}
			}
			else
			{
				// Altrimenti setto NULL per uscire dal ciclo
				free(blockHeader);
				blockHeader = NULL;
			}
		}
		
		// Deallocazione array dei nomi dei file della directory
		/*i = 0;
		while ( i < d->fdb->num_entries )
		{
			//fprintf(stderr, "SimpleFS_createFile() -> Dealloco posizione %d\n", i);
			free(contenutoDirectory[i]);
			//fprintf(stderr, "SimpleFS_createFile() -> Deallocato posizione %d\n", i);
			i++;

		}*/
	}
	
	if ( indexInode != -1)
	{
		inodeRead = malloc(sizeof(Inode));
		if ( DiskDriver_readInode(d->sfs->disk, inodeRead, indexInode) != -1 )
		{
			// Aggiorno l'inode, ho il suo indice in index_inode
			inodeRead->dataUltimoAccesso = time(NULL);
			if ( DiskDriver_writeInode(d->sfs->disk, inodeRead, indexInode) != -1 )
			{
				// OK
				fprintf(stderr, "SimpleFS_changeDir() -> aggiornato data ultimo accesso di inode %d\n", indexInode);
			}
			else
			{
				fprintf(stderr, "SimpleFS_changeDir() -> errore nell'aggiornamento data ultimo accesso di inode %d\n", indexInode);
			}
		}
		else
		{
			fprintf(stderr, "SimpleFS_changeDir() -> errore nella lettura per l'aggiornamento data ultimo accesso di inode %d\n", indexInode);
		}
	}
	DiskDriver_flush(d->sfs->disk);
	return ret;
}
/*
int SimpleFS_changeDir(DirectoryHandle* d, char* dirname) {
	char** contenutoDirectory = (char**)malloc(sizeof(d->fdb->num_entries*sizeof(char*)));
	i = 0;
	while ( i < d->fdb->num_entries ) {
		contenutoDirectory[i] = (char*)malloc(128*sizeof(char));
		i++;
	}
	SimpleFS_readDir(contenutoDirectory, d);
	int presente=0;
	i=0;
	while (i < d->fdb->num_entries) {
		if (contenutoDirectory[i]==dirname) {
			presente++;
			break;
		}
	}
	if (presente==0) {
		printf("la cartella selezionata non è presente");
		return -1;
	}
}
*/
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
	Inode* inode = NULL;
	int indexInode;
	int indexData;
	int i;
	int ret = -1;
	int exist = 0;
	int dimensioneArray;
	FirstDirectoryBlock* firstDirectoryBlock = NULL;
	// Inizializzaizone per registrare i nomi della directory
	char** contenutoDirectory = (char**)malloc(d->fdb->num_entries*sizeof(char*));
	i = 0;
	dimensioneArray = d->fdb->num_entries;
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
		if (strncmp(dirname, contenutoDirectory[i], 128) == 0 )
		{
			exist = 1;
		}
		i++;
	}
	free(contenutoDirectory);
	if ( exist == 0 )
	{
		if ( d->sfs->disk->header->inodeFree_blocks > 0 ) {
			//indexInode=d->sfs->disk->header->inodeFirst_free_block;
			indexInode = BitMap_get(bitmapInode, 0, 0);
			if (d->sfs->disk->header->dataFree_blocks>0) {
				//indexData= d->sfs->disk->header->dataFirst_free_block;
				indexData = BitMap_get(bitmapData, 0, 0);

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
				//memset(firstDirectoryBlock->file_inodes, -1, (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)-sizeof(int))/sizeof(int) );
				i = 0;
				while ( i < (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)-sizeof(int))/sizeof(int) )
				{
					firstDirectoryBlock->file_inodes[i] = -1;
					i++;
				}

				if ( DiskDriver_writeInode(d->sfs->disk, inode, indexInode)!= -1 ) {
					fprintf(stderr, "SimpleFS_mkdir() -> scritto su inode: %d\n", indexInode); 
					if (DiskDriver_writeBlock(d->sfs->disk, (void*)firstDirectoryBlock, indexData) !=-1) {
						fprintf(stderr, "SimpleFS_mkdir() -> scritto su blocco: %d\n", indexData); 
						if (SimpleFS_insertInodeInDirectory(d,indexInode) != -1 ) {
							d->fdb->num_entries= d->fdb->num_entries + 1;
							// E la riscrivo su file
							if ( DiskDriver_writeBlock(d->sfs->disk, d->fdb, d->fdb->fcb.block_in_disk) != - 1)
							{
								// OK
								fprintf(stderr, "SimpleFS_mkDir() -> Ora ci sono %d files\n", d->fdb->num_entries);
								// Aggiorno la data ultima modifica dell'inode della cartella padre
								if ( DiskDriver_readInode(d->sfs->disk, inode, d->inode) != -1 )
								{
									// Modifico la data ultima modifica dell'inode
									inode->dataUltimaModifica = time(NULL);
									// Riscrivo l'inode
									if ( DiskDriver_writeInode(d->sfs->disk, inode, d->inode) != -1)
									{
										ret = 0;
									}
									else
									{
										fprintf(stderr, "SimpleFS_mkdir() -> errore durante la scrittura dell'aggiornamento ultima modifica dell'inode %d\n", d->inode);
									}
								}
								else
								{
									fprintf(stderr, "SimpleFS_mkdir() -> errore durante la lettura per l'aggiornamento ultima modifica dell'inode %d\n", d->inode);
								}
								//DiskDriver_flush(d->sfs->disk);
								//return 0;
							}
							else
							{
								fprintf(stderr, "SimpleFS_mkDir() -> Errore durante la sovrascrittura del FirstDirectoryBlock della cartella padre\n");
							}

						}
						else  {
							fprintf(stderr, "SimpleFS_mkDir impossibile inserire inode nella directory d \n");
							//return -1;
						}
					}
					else
					{
						fprintf(stderr, "SimpleFS_mkDir() -> Errore durante la scruttura del nuovo blocco FirstDirectoryBlock\n");
					}

				}
				else {
					fprintf(stderr, "Errore durante la scrittura dell'inode\n");
					//return -1;
				}
			}
		}
	}
	else
	{
		fprintf(stderr, "SimpleFS_mkDir() -> La directory già esiste\n");
	}
	//return -1; 
	if ( inode != NULL ) free(inode);
	if ( firstDirectoryBlock != NULL ) free(firstDirectoryBlock);
	DiskDriver_flush(d->sfs->disk);
	return ret;
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



void static printPermessi(unsigned char u,unsigned char g,unsigned char o){
	(bit_is_one(u,5))?fprintf(stderr," r"):fprintf(stderr," -");
	(bit_is_one(u,6))?fprintf(stderr,"w"):fprintf(stderr,"-");
	(bit_is_one(u,7))?fprintf(stderr,"x"):fprintf(stderr,"-");
	
	(bit_is_one(g,5))?fprintf(stderr,"r"):fprintf(stderr,"-");
	(bit_is_one(g,6))?fprintf(stderr,"w"):fprintf(stderr,"-");
	(bit_is_one(g,7))?fprintf(stderr,"x"):fprintf(stderr,"-");
	
	(bit_is_one(o,5))?fprintf(stderr,"r"):fprintf(stderr,"-");
	(bit_is_one(o,6))?fprintf(stderr,"w"):fprintf(stderr,"-");
	(bit_is_one(o,7))?fprintf(stderr,"x"):fprintf(stderr,"-");
	/*
	printf("%d%d%d",u,g,o);
	printf("\n");
	bits_print(&u,1);
	bits_print(&g,1);
	bits_print(&o,1);
	*/
	}

//list (print names of) all files (both r and d types) in current directory 
//return 0 on success else -1
int SimpleFS_listFiles(SimpleFS* fs){
	int ret;
	
	int limit= (BLOCK_SIZE
		   -sizeof(BlockHeader)
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
	
	Inode* inode=malloc(sizeof(Inode));
	ret=DiskDriver_readInode(fs->disk,inode,fs->current_directory_inode);
	if(ret<0) return -1;
	//inode della cur dir caricato
	
	FirstDirectoryBlock* fdb=malloc(sizeof(FirstDirectoryBlock));
	ret=DiskDriver_readBlock(fs->disk,fdb,inode->primoBlocco);
	if(ret<0) return -1;
	//fdb caricato
	
	int i;
	int j;
	DirectoryBlock* db= malloc(sizeof(DirectoryBlock));
	FirstFileBlock* ffb= malloc(sizeof(FirstFileBlock));
	fprintf(stderr,"%s -> listing content of %s \n",__func__,fdb->fcb.name);
	int first_block_flag=TRUE;
	int n_block=-1;
	
	//for(j=0; j < fdb->num_entries; j++){
	j = 0;
	i = 0;
	while ( j < fdb->num_entries )
	{	
		//fprintf(stderr,"%s -> ",__func__);
		
		if(i<limit && first_block_flag){
			//fprintf(stderr,"%d\n",fdb->file_inodes[i]);
			//if(fdb->file_inodes[i]<0) continue;
			if ( fdb->file_inodes[i] != - 1)
			{
				ret=DiskDriver_readInode(fs->disk,inode,fdb->file_inodes[i] );
				if(ret<0) return -1;
				ret=DiskDriver_readBlock(fs->disk,ffb,inode->primoBlocco);
				if(ret<0) return -1;
				printPermessi(inode->permessiUtente,inode->permessiGruppo,inode->permessiAltri);
				j++;
				//fprintf(stderr, "Data ultima modifica %ld, data ultimo accesso %ld, data creazione %ld\n", inode->dataUltimaModifica, inode->dataUltimoAccesso, inode->dataCreazione);
				//fprintf(stderr, "Data strutturata ultima modifica: %s", ctime(&(inode->dataUltimaModifica)));
				//fprintf(stderr, "Data strutturata ultimo accesso: %s", ctime(&(inode->dataUltimoAccesso)));
				//fprintf(stderr, "Data strutturata datacreazione: %s", ctime(&(inode->dataCreazione)));
				if(inode->tipoFile=='r')
				{
					//fprintf(stderr," %c %ld %.5s %.5s %.5s %s \n" ,inode->tipoFile,inode->dimensioneFile,ctime(&(inode->dataCreazione))+11,
						//ctime(&(inode->dataUltimoAccesso))+11,ctime(&(inode->dataUltimaModifica))+11,ffb->fcb.name);
					fprintf(stderr,"%c %ld " ,inode->tipoFile,inode->dimensioneFile);
					//ctime(&(inode->dataUltimoAccesso))+11,ctime(&(inode->dataUltimaModifica))+11,ffb->fcb.name);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataCreazione))+11);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataUltimoAccesso))+11);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataUltimaModifica))+11);
					fprintf(stderr,"%s\n", ffb->fcb.name);
				}
				else
				{
					//fprintf(stderr," %c %ld %.5s %.5s %.5s"  ANSI_COLOR_CYAN " %s" ANSI_COLOR_RESET "\n" ,inode->tipoFile,inode->dimensioneFile,ctime(&(inode->dataCreazione))+11,
						//ctime(&(inode->dataUltimoAccesso))+11,ctime(&(inode->dataUltimaModifica))+11,ffb->fcb.name);
					fprintf(stderr,"%c %ld " ,inode->tipoFile,inode->dimensioneFile);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataCreazione))+11);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataUltimoAccesso))+11);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataUltimaModifica))+11);
					fprintf(stderr, ANSI_COLOR_CYAN "%s" ANSI_COLOR_RESET "\n", ffb->fcb.name);
				}
			}
		}
		if(i>=limit && first_block_flag){	//c è bisogno di altri  blocchi (db) e venivamo da fdb
			first_block_flag=FALSE;
			n_block=fdb->header.next_block;
			if(n_block==-1) break;
			ret=DiskDriver_readBlock(fs->disk,db,n_block);	//caricato il db
			if(ret<0) return -1;
			//if(db->file_inodes[(i-limit)%(sizeof(DirectoryBlock)-sizeof(BlockHeader))] < 0) continue;
			if ( db->file_inodes[(i-limit)%(sizeof(DirectoryBlock)-sizeof(BlockHeader))] != -1 )
			{
				ret=DiskDriver_readInode(fs->disk,ffb,db->file_inodes[(i-limit)%(sizeof(DirectoryBlock)-sizeof(BlockHeader))] );
				if(ret<0) return -1;
				printPermessi(inode->permessiUtente,inode->permessiGruppo,inode->permessiAltri);
				j++;
				if(inode->tipoFile=='r')
				{
					//fprintf(stderr," %c %ld %.5s %.5s %.5s %s \n" ,inode->tipoFile,inode->dimensioneFile,ctime(&(inode->dataCreazione))+11,
					fprintf(stderr," %c %ld " ,inode->tipoFile,inode->dimensioneFile);
						//ctime(&(inode->dataUltimoAccesso))+11,ctime(&(inode->dataUltimaModifica))+11,ffb->fcb.name);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataCreazione))+11);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataUltimoAccesso))+11);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataUltimaModifica))+11);
					fprintf(stderr,"%s\n", ffb->fcb.name);
					
				}
				else
				{
					//fprintf(stderr," %c %ld %.5s %.5s %.5s" ANSI_COLOR_CYAN " %s" ANSI_COLOR_RESET "\n" ,inode->tipoFile,inode->dimensioneFile,ctime(&(inode->dataCreazione))+11,
						//ctime(&(inode->dataUltimoAccesso))+11,ctime(&(inode->dataUltimaModifica))+11,ffb->fcb.name);
					fprintf(stderr," %c %ld " ,inode->tipoFile,inode->dimensioneFile);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataCreazione))+11);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataUltimoAccesso))+11);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataUltimaModifica))+11);
					fprintf(stderr, ANSI_COLOR_CYAN " %s" ANSI_COLOR_RESET "\n", ffb->fcb.name);
				}
			}
		}
		else if(i>=limit){
			n_block=db->header.next_block;
			if(n_block==-1) break;
			ret=DiskDriver_readBlock(fs->disk,db,n_block);
			if(ret<0) return -1;
			//if(db->file_inodes[(i-limit)%(sizeof(DirectoryBlock)-sizeof(BlockHeader))] < 0) continue;
			if ( db->file_inodes[(i-limit)%(sizeof(DirectoryBlock)-sizeof(BlockHeader))] != -1 )
			{
				ret=DiskDriver_readInode(fs->disk,ffb,db->file_inodes[(i-limit)%(sizeof(DirectoryBlock)-sizeof(BlockHeader))] );
				if(ret<0) return -1;
				printPermessi(inode->permessiUtente,inode->permessiGruppo,inode->permessiAltri);
				j++;
				if(inode->tipoFile=='r')
				{
					//	fprintf(stderr," %c %ld %.8s %.8s %.8s %s \n" ,inode->tipoFile,inode->dimensioneFile,ctime(&(inode->dataCreazione))+11,
					//		ctime(&(inode->dataUltimoAccesso))+11,ctime(&(inode->dataUltimaModifica))+11,ffb->fcb.name);
					fprintf(stderr," %c %ld " ,inode->tipoFile,inode->dimensioneFile);
						//ctime(&(inode->dataUltimoAccesso))+11,ctime(&(inode->dataUltimaModifica))+11,ffb->fcb.name);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataCreazione))+11);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataUltimoAccesso))+11);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataUltimaModifica))+11);
					fprintf(stderr,"%s\n", ffb->fcb.name);
				}
				else
				{
					//fprintf(stderr," %c %ld %.8s %.8s %.8s" ANSI_COLOR_CYAN " %s" ANSI_COLOR_RESET "\n" ,inode->tipoFile,inode->dimensioneFile,ctime(&(inode->dataCreazione))+11,
						//ctime(&(inode->dataUltimoAccesso))+11,ctime(&(inode->dataUltimaModifica))+11,ffb->fcb.name);		
					fprintf(stderr," %c %ld " ,inode->tipoFile,inode->dimensioneFile);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataCreazione))+11);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataUltimoAccesso))+11);
					fprintf(stderr,"%.8s ", ctime(&(inode->dataUltimaModifica))+11);
					fprintf(stderr, ANSI_COLOR_CYAN " %s" ANSI_COLOR_RESET "\n", ffb->fcb.name);
				}
			}
		}
		i++;
	}
		
	return 0;
	
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
//static int single_remove(SimpleFS* fs,FirstFileBlock* ffb,Inode* inode){
static int single_remove(DirectoryHandle* d,FirstFileBlock* ffb,Inode* inode){
	int ret;
	fprintf(stderr, "single_remove() -> Libero il primo blocco data %d\n", inode->primoBlocco);
	ret=DiskDriver_freeBlock(d->sfs->disk,inode->primoBlocco);
	if(ret<0) return -1;
	FileBlock* fb;
	int next_b=ffb->header.next_block;
	while(next_b != -1){ //se il file ha piu di un blocco ,li cancello tutti
			fb=malloc(sizeof(FileBlock));
			ret = DiskDriver_readBlock(d->sfs->disk, fb, next_b);
			if ( ret<0){
				free(fb);
				return -1;
			}
			//libero il next_b-esimo blocco del file
			fprintf(stderr, "single_remove() -> Libero il blocco data %d\n", next_b);
			ret=DiskDriver_freeBlock(d->sfs->disk,next_b);
			if(ret<0){
				free(fb);
				return -1;
			}
			//passo al prox
			next_b=fb->header.next_block;
			fprintf(stderr, "single_remove() -> prossimo blocco data da liberare %d\n", next_b);
			free(fb);
			
		}

		
	return 0;
}



/*
Pseudo code of rec remove:

rec_remove(d){
	
	
	for( ogni blocco di d){		// fdb-> db -> db ...  
		for( ogni figlio nel blocco){
			
			if(inode del figlio=='r'){
				single_remove(inode del figlio -> primoBlocco)
				DiskDriver_freeInode(d->sfs->disk,files_inode[figlio] )
				num_removed++;
			}
			else 
				rec_remove(figlio,num_removed) //fdb/db
		}
		next=blocco->header.next_block
		DiskDriver_freeBlock(disk,next);//< fdb/db	,libero il blocco della cartella 
		num_removed++;
		DiskDriver_freeInode(disk,files_inodes[figlio])
	}
	return;
	
	}
	 
	 
	
*/

		









static int rec_remove(DirectoryHandle* d,FirstDirectoryBlock* fdbr, Inode* inode, int num_removed){
	int ret;
	int limit;
	
	
	Inode* in=malloc(sizeof(Inode));
	FirstDirectoryBlock* fdb=malloc(sizeof(FirstDirectoryBlock));
	DirectoryBlock* db=malloc(sizeof(DirectoryBlock));
	FirstFileBlock* ffb=malloc(sizeof(FirstFileBlock));
	
	int i,inode_index,n_block=fdbr->header.block_in_file;
	unsigned char fdb_gone=FALSE;
	while( n_block >= 0){
		
		if(fdb_gone ){
			ret= DiskDriver_readBlock(d->sfs->disk,db,n_block);
			if(ret<0) return -1;
			
			limit=(BLOCK_SIZE	//limit di db
		   -sizeof(BlockHeader)
		    -sizeof(int))/sizeof(int);
		
		}
		else{
			//fdb_gone=TRUE;
			
			limit=(BLOCK_SIZE		//limit di fdb
		   -sizeof(BlockHeader)
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
		}
		
		for(i=0;i < limit; i++){
			if(! fdb_gone)
			{
				//fprintf(stderr, "rec_remove() -> FDB: nome file = %s\n", fdbr->fcb.name);
				inode_index=fdbr->file_inodes[i];
				//fdb_gone = TRUE;
			}
			else
			{
				//fprintf(stderr, "rec_remove() -> DB: nome file = %s\n", db->fcb.name);
				fprintf(stderr, "rec_remove() -> DB: sono qui\n");
				inode_index=db->file_inodes[i];
			}
			
			if ( inode_index != - 1)
			{
				ret= DiskDriver_readInode(d->sfs->disk,in,inode_index);
				fprintf(stderr, "rec_remove() -> inode letto: %d\n", inode_index);
				if(ret<0){
					fprintf(stderr, "rec_remove() -> errore durante la lettura dell'inode %d\n", inode_index);
					return -1;
				}
				//fprintf(stderr, "rec_remove() -> esci prima?\n");
				if(in->tipoFile=='r'){
					ret= DiskDriver_readBlock(d->sfs->disk,ffb,in->primoBlocco);
					if(ret<0) {
						fprintf(stderr, "rec_remove() -> errore durante la lettura del primoblocco data %d\n", in->primoBlocco);
						return -1;
					}
					single_remove(d,ffb,in);
					fprintf(stderr,"%s -> rimosso file %s \n",__func__,ffb->fcb.name);
					ret= DiskDriver_freeInode(d->sfs->disk, inode_index);
					if(ret<0) {
						fprintf(stderr, "rec_remove() -> errore durante la liberazione dell'inode %d\n", inode_index);
						return -1;
					}
					num_removed--;
					
				}
				else if (in->tipoFile=='d'){
					
					ret= DiskDriver_readBlock(d->sfs->disk,fdb,in->primoBlocco);
					if(ret<0) return -1;
					fprintf(stderr,"%s -> siamo entrati nella directory %s \n",__func__,fdb->fcb.name);
					rec_remove(d,fdb,in,num_removed);
					fprintf(stderr, "rec_remove() -> Libero il blocco data %d\n", in->primoBlocco);
					//ret=DiskDriver_freeBlock(d->sfs->disk,n_block);	//cancella fdb o db perche ha percorso tutti i files_inodes[]
					ret=DiskDriver_freeBlock(d->sfs->disk,in->primoBlocco);	//cancella fdb o db perche ha percorso tutti i files_inodes[]
					if(ret<0) {
						fprintf(stderr, "rec_remove() -> Errore durante liberazione blocco data %d\n", n_block);
						return -1;
					}
					fprintf(stderr, "rec_remove() -> Libero inode %d\n", inode_index);
					ret=DiskDriver_freeInode(d->sfs->disk,inode_index);
					if(ret<0) 
					{
						fprintf(stderr, "rec_remove() -> Errore durante liberazione inode %d\n", inode_index);
						return -1;
					}
					
				}
				else{
					fprintf(stderr,"%s -> tipo file not recognized %d \n",__func__,inode_index);
					return -1;
					}
			 }
		}
		
		/*fprintf(stderr, "rec_remove() -> Libero il blocco data %d\n", n_block);
		ret=DiskDriver_freeBlock(d->sfs->disk,n_block);	//cancella fdb o db perche ha percorso tutti i files_inodes[]
		if(ret<0) return -1;*/
		//fprintf(stderr,"%s -> rimossa directory %s \n",__func__,fdb->fcb.name);
		if(fdb_gone){
			n_block=db->header.next_block;
		}
		else{
			n_block=fdbr->header.next_block;
			fdb_gone = TRUE;
		}
		num_removed++;
		
		/*ret=DiskDriver_freeInode(d->sfs->disk,inode_index);
		if(ret<0) return -1;*/
		
			
			
	}
		
		
	
	free(ffb);
	free(db);
	free(fdb);
	free(in);
	
	
	return num_removed;
	
}



int simpleFS_remove_corta(DirectoryHandle* d ,char* filename){
	
	int ret;
	if (d->sfs==NULL || d->sfs->disk==NULL || filename==NULL || strcmp(filename,"")==0){
		perror("parameter(s) not correct");
		return -1;
		}
	int in_index = d->inode;
	assert(in_index >= 0);
	
	Inode* inode=malloc(sizeof(Inode));
	
	//scorro la dir. corrente per vedere se trovo il file con name filename e se è un file o una dir
	int i;
	int limit=(BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
	
	
	DirectoryBlock* db= malloc(sizeof(DirectoryBlock));	//alloco per dopo
	FirstFileBlock* ffb= malloc(sizeof(FirstFileBlock));
	FirstDirectoryBlock* fdb= malloc(sizeof(FirstDirectoryBlock));
	unsigned char found_file=FALSE;
	unsigned char no_db=FALSE;
	int n_block=0,num_db=0,parent_in_index=0;
	

	if(d->fdb->num_entries <= limit){	//basta il fdb
		no_db=TRUE;
	}
	else{  //c è bisogno anche dei db
		no_db=FALSE;
		//calcolo il numero di db che devo scorrere
		num_db= (int)ceil( (double) (d->fdb->num_entries-limit) / ((BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int)) );
	}


	while(n_block >= 0){
	
	
		if(no_db==TRUE){ //mi basta scorrere tutti i num_entries file_inodes[] di  fdbr
			limit=d->fdb->num_entries;
			}
		else{
			ret=DiskDriver_readBlock(d->sfs->disk,db,n_block);
			if(ret<0) return -1;
			
			limit= (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
			}
		
		for(i=0; i< limit; i++){	
			if(no_db){
				ret = DiskDriver_readInode(d->sfs->disk, inode, d->fdb->file_inodes[i]);
				if(ret<0) return -1;
				if(d->fdb->file_inodes[i] < 0) continue;
				parent_in_index=d->fdb->file_inodes[i];
			}
			else{
				ret = DiskDriver_readInode(d->sfs->disk, inode, db->file_inodes[i]);
				if(ret<0) return -1;
				if(db->file_inodes[i] < 0) continue;
				parent_in_index=db->file_inodes[i];
				}
			
			if(inode->tipoFile=='r'){ //è un file
				ret=DiskDriver_readBlock(d->sfs->disk,ffb,inode->primoBlocco);	//mi basta il primo blocco quello con fcb
				if(strcmp(ffb->fcb.name,filename)==0){	//trovato il file e basta rimuoverlo
					found_file=TRUE;
					fprintf(stderr,"%s -> trovato il file %s \n",__func__,ffb->fcb.name);
					single_remove(d,ffb,inode);
					printf("SimpleFS_remove() -> sono nella cartella %s\n", d->fdb->fcb.name);
					//fdbr->file_inodes[i]=-1;
					// Libero l'inode dall'inodemap
					if(no_db){
						ret = DiskDriver_freeInode(d->sfs->disk, d->fdb->file_inodes[i]);
						if ( ret < 0 ) return -1;
						d->fdb->file_inodes[i]=-1;
						// Decremento il numero di file all'interno della cartella
						d->fdb->num_entries--;
						fprintf(stderr, "SimpleFS_remove() -> setto a -1 il file_inodes[%d]\n", i);
						ret = DiskDriver_writeBlock(d->sfs->disk, d->fdb, d->fdb->fcb.block_in_disk);
						//fprintf(stderr, "SimpleFS_remove() -> aggiorno il blocco del padre: %d", inode->primoBlocco);
						if(ret<0) return -1;
					}
					else{
						ret = DiskDriver_freeInode(d->sfs->disk, db->file_inodes[i]);
						if ( ret < 0 ) return -1;
						db->file_inodes[i]=-1;
						// Decremento il numero di file all'interno della cartella
						//TODO num_entries nel fdb --
						fprintf(stderr, "SimpleFS_remove() -> [DB] setto a -1 il file_inodes[%d]\n", i);
						//fprintf(stderr, "SimpleFS_remove() -> aggiorno il blocco del padre: %d", inode->primoBlocco);
						
						ret = DiskDriver_writeBlock(d->sfs->disk, db, n_block);
						if(ret<0) return -1;
						//aggiorno blocco per file_inodes cambiata
						
						}
					break;
					}
			}
			else if (inode->tipoFile=='d'){	//è una dir
				
				ret=DiskDriver_readBlock(d->sfs->disk,fdb,inode->primoBlocco);
				if(ret<0) return -1;
				if(strcmp(fdb->fcb.name,filename)==0){	//trovata la dir con name filename , eliminaz. ricorsiva
					found_file=TRUE;
					fprintf(stderr,"%s -> trovata la directory %s \n",__func__,fdb->fcb.name);
					rec_remove(d,fdb,inode,0);
					fprintf(stderr, "SimpleFs_remove() -> Libero il blocco data %d\n", n_block);
					if(no_db){
						ret=DiskDriver_freeBlock(d->sfs->disk,d->fdb->fcb.block_in_disk);
						if(ret<0) return -1;
						ret=DiskDriver_freeInode(d->sfs->disk,parent_in_index);
						if(ret<0) return -1;
					}
					else{
						ret=DiskDriver_freeBlock(d->sfs->disk,n_block);	//cancella fdb o db perche ha percorso tutti i files_inodes[]
						if(ret<0) return -1;
						ret=DiskDriver_freeInode(d->sfs->disk,parent_in_index);
						if(ret<0) return -1;
					}
					
					break;
					}
				}
			else{
				perror("SimpleFS_remove() -> not recognized tipoFile");
				return -1;
				}
		}
	
		if(n_block==0)
			n_block=fdb->header.next_block;
		else
			n_block=db->header.next_block;
		
	}

	
	
	free(ffb);	
	free(fdb);
	free(db);
	//free(fdbr);
	free(inode);
	
	if(!found_file){
		fprintf(stderr,"SimpleFS_remove() -> file not found \n");
		return -1;
	}
	fprintf(stderr,"%s -> eliminazione completata \n",__func__);
	return 0;
	

	
}


// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files, both r 
// files and d files with all its children...
// if a file removes simply that file
// NB: if a file is removed in a dir, you have to put -1  
// value in file_inodes[i] of that dir, at the index i of the file
//int SimpleFS_remove(SimpleFS* fs, char* filename){
int SimpleFS_remove(DirectoryHandle* d, char* filename){
	int ret;
	if (d->sfs==NULL || d->sfs->disk==NULL || filename==NULL || strcmp(filename,"")==0){
		perror("parameter(s) not correct");
		return -1;
		}
	//int in_index=fs->current_directory_inode;
	int in_index = d->inode;
	assert(in_index >= 0);
	
	Inode* inode=malloc(sizeof(Inode));
	/*
	ret= DiskDriver_readInode(fs->disk,inode,in_index);	//leggo l' inode della cur. dir.
	if(ret<0) return -1;
	fprintf(stderr,"SimpleFS_remove() -> Letto inode %d della current directory\n",in_index);
	
	assert(inode!=NULL);
	assert(inode->tipoFile=='d');
	
	//prendo l' indice del primo blocco di tipo FirstDirectoryBlock 
	int block_index=inode->primoBlocco;

	//parto dal primo blocco (per forza FirstDirectory perche cur. dir è una dir)
	FirstDirectoryBlock* fdbr=malloc(sizeof(FirstDirectoryBlock));
	ret=DiskDriver_readBlock(fs->disk,fdbr,block_index);	// leggo il primo blocco dir. della cur dir
	if(ret<0) return -1;
	assert(fdbr!=NULL);
	*/
	//scorro la dir. corrente per vedere se trovo il file con name filename e se è un file o una dir
	int i;
	int limit= (BLOCK_SIZE
		   -sizeof(BlockHeader)
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
	
	
	DirectoryBlock* dbr= malloc(sizeof(DirectoryBlock));	//alloco per dopo
	FirstFileBlock* ffb= malloc(sizeof(FirstFileBlock));
	FirstDirectoryBlock* fdb= malloc(sizeof(FirstDirectoryBlock));
	unsigned char found_file=FALSE;
	unsigned char no_db=FALSE;
	int n_block=0,num_db=0;
	

	if(d->fdb->num_entries <= limit){	//basta il fdb
		no_db=TRUE;
	}
	else{  //c è bisogno anche dei db
		no_db=FALSE;
		//calcolo il numero di db che devo scorrere
		//num_db= ceil( (fdbr->num_entries-limit) / ((BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int)) );
		num_db= (int)ceil( (double) (d->fdb->num_entries-limit) / ((BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int)) );
	}

	if(no_db==TRUE){	//mi basta scorrere tutti i num_entries file_inodes[] di  fdbr
		//for(i=0;i< fdbr->num_entries; i++){
		//for(i=0; i< d->fdb->num_entries; i++){	
		for ( i=0; i < limit; i ++ ){
			if ( d->fdb->file_inodes[i] != -1 )
			{
				//ret=DiskDriver_readInode(fs->disk,inode,fdbr->file_inodes[i]);
				ret = DiskDriver_readInode(d->sfs->disk, inode, d->fdb->file_inodes[i]);
				if(ret<0) return -1;
				//if(fdbr->file_inodes[i] < 0) continue;
				//if(d->fdb->file_inodes[i] < 0) continue;
				
				if(inode->tipoFile=='r'){ //è un file
					fprintf(stderr, "SimpleFS_remove() -> FDB: file_inodes[%d] è un file\n", i);
					ret=DiskDriver_readBlock(d->sfs->disk,ffb,inode->primoBlocco);	//mi basta il primo blocco quello con fcb
					if(ret<0) return -1;
					if(strcmp(ffb->fcb.name,filename)==0){	//trovato il file e basta rimuoverlo
						found_file=TRUE;
						fprintf(stderr,"%s -> trovato il file %s \n",__func__,ffb->fcb.name);
						single_remove(d,ffb,inode);
						printf("SimpleFS_remove() -> sono nella cartella %s\n", d->fdb->fcb.name);
						//fdbr->file_inodes[i]=-1;
						// Libero l'inode dall'inodemap
						ret = DiskDriver_freeInode(d->sfs->disk, d->fdb->file_inodes[i]);
						if ( ret < 0 ) return -1;
						d->fdb->file_inodes[i]=-1;
						fprintf(stderr, "SimpleFS_remove() -> setto a -1 il file_inodes[%d]\n", i);
						// Decremento il numero di file all'interno della cartella
						d->fdb->num_entries--;
						fprintf(stderr, "SimpleFS_remove() -> Libero il blocco data %d\n", inode->primoBlocco);
						ret = DiskDriver_freeBlock(d->sfs->disk, inode->primoBlocco);
						if(ret<0) return -1;
						
						//ret=DiskDriver_writeBlock(fs->disk,fdbr,inode->primoBlocco);	//aggiorno la fdbr con la file_inodes cambiata
						ret = DiskDriver_writeBlock(d->sfs->disk, d->fdb, d->fdb->fcb.block_in_disk);
						//fprintf(stderr, "SimpleFS_remove() -> aggiorno il blocco del padre: %d", inode->primoBlocco);
						if(ret<0) return -1;
						
						break;
					}
					else
					{
						//fprintf(stderr, "SimpleFS_remove() -> Non è questo il file cercato: %s\n", ffb->fcb.name);
					}
				}
				else if (inode->tipoFile=='d'){	//è una dir
					fprintf(stderr, "SimpleFS_remove() -> FDB: file_inodes[%d] è una cartella\n", i);
					//ret=DiskDriver_readBlock(fs->disk,fdb,inode->primoBlocco);
					ret=DiskDriver_readBlock(d->sfs->disk,fdb,inode->primoBlocco);
					if(ret<0)
					{ 
						fprintf(stderr, "SimpleFS_remove() -> Errore durante lettura blocco data %d\n", inode->primoBlocco);
						return -1;
					}
					fprintf(stderr, "SimpleFS_remove() -> Il file che sto analizzando ha nome: %s\n", fdb->fcb.name);
					if(strcmp(fdb->fcb.name,filename)==0){	//trovata la dir con name filename , eliminaz. ricorsiva
						found_file=TRUE;
						fprintf(stderr,"%s -> trovata la directory %s \n",__func__,fdb->fcb.name);
						rec_remove(d,fdb,inode,0);
						//fprintf(stderr, "SimpleFs_remove() -> Libero il blocco data %d\n", n_block);
						fprintf(stderr, "SimpleFS_remove() -> Libero l'inode %d\n", d->fdb->file_inodes[i]);
						ret=DiskDriver_freeInode(d->sfs->disk,d->fdb->file_inodes[i]);
						if(ret<0) {
							fprintf(stderr, "SimpleFS_remove() -> Errore durante liberazione inode %d\n", d->fdb->file_inodes[i]);
							return -1;
						}
						d->fdb->file_inodes[i]=-1;
						// Decremento il numero di file all'interno della cartella
						d->fdb->num_entries--;
						fprintf(stderr, "SimpleFS_remove() -> Libero il blocco data %d\n", inode->primoBlocco);
						ret = DiskDriver_freeBlock(d->sfs->disk, inode->primoBlocco);
						if (ret<0) return -1;
						//ret=DiskDriver_freeBlock(d->sfs->disk,n_block);	//cancella fdb o db perche ha percorso tutti i files_inodes[]
						//if(ret<0) return -1;
						/*ret=DiskDriver_freeInode(d->sfs->disk,d->fdb->file_inodes[i]);
						if(ret<0) {
							fprintf(stderr, "SimpleFS_remove() -> Errore durante liberazione inode %d\n", d->fdb->file_inodes[i]);
							return -1;
						}*/
						// Devo scrivere le modifiche del FDB (num_entries--)
						ret = DiskDriver_writeBlock(d->sfs->disk, d->fdb, d->fdb->fcb.block_in_disk);
						if(ret<0) return -1;
						break;
					}
					else
					{
						//fprintf(stderr, "SimpleFS_remove() -> Non è questo la cartella cercata: %s\n", fdb->fcb.name);
					}
				}
				else{
					perror("SimpleFS_remove() -> not recognized tipoFile");
					return -1;
				}
			}
			else
			{
				fprintf(stderr, "SimpleFS_remove() -> FDB: file_inodes[%d] è -1\n", i);
			}
		}
	}
	else{	//c è bisogno dei db
		fprintf(stderr,"%s -> Necessaria scansione dei db successivi (num_entries>limit) \n",__func__);
		for(i=0;i< limit; i++){	//leggo sempre tutti i file_inodes[] del primo blocco
			if ( d->fdb->file_inodes[i] != -1 )
			{
				//ret=DiskDriver_readInode(fs->disk,inode,fdbr->file_inodes[i]);
				ret=DiskDriver_readInode(d->sfs->disk,inode,d->fdb->file_inodes[i]);
				if(ret<0) return -1;
				//if(fdbr->file_inodes[i] < 0) continue;
				if(d->fdb->file_inodes[i] < 0) continue;
				
				if(inode->tipoFile=='r'){ //è un file
					//ret=DiskDriver_readBlock(fs->disk,ffb,inode->primoBlocco);	//mi basta il primo blocco quello con fcb
					ret=DiskDriver_readBlock(d->sfs->disk,ffb,inode->primoBlocco);	//mi basta il primo blocco quello con fcb
					if(strcmp(ffb->fcb.name,filename)==0){	//trovato il file e basta rimuoverlo
						found_file=TRUE;
						fprintf(stderr,"%s -> trovato il file %s \n",__func__,ffb->fcb.name);
						single_remove(d,ffb,inode);
						fprintf(stderr, "SimpleFS_remove() ->DB: Libero l'inode %d\n", d->fdb->file_inodes[i]);
						ret = DiskDriver_freeInode(d->sfs->disk, d->fdb->file_inodes[i]);
						fprintf(stderr, "SimpleFS_remove() -> DB: Ho liberato l'inode %d\n", d->fdb->file_inodes[i]);
						if ( ret < 0 ) return -1;
						d->fdb->file_inodes[i]=-1;
						// Decremento il numero di file all'interno della cartella
						d->fdb->num_entries--;
						fprintf(stderr, "SimpleFS_remove() -> DB: Libero il blocco data %d\n", inode->primoBlocco);
						ret = DiskDriver_freeBlock(d->sfs->disk, inode->primoBlocco);
						if(ret<0) return -1;
						//ret=DiskDriver_writeBlock(d->sfs->disk,d->fdb,d->);	//aggiorno la fdbr con la file_inodes cambiata
						ret = DiskDriver_writeBlock(d->sfs->disk, d->fdb, d->fdb->fcb.block_in_disk);
						if(ret<0) return -1;
						break;
						}
				}
				else if (inode->tipoFile=='d'){	//è una dir
					//ret=DiskDriver_readBlock(fs->disk,fdb,inode->primoBlocco);
					ret=DiskDriver_readBlock(d->sfs->disk,fdb,inode->primoBlocco);
					if(ret<0) return -1;
					if(strcmp(fdb->fcb.name,filename)==0){	//trovata la dir con name filename , eliminaz. ricorsiva
						found_file=TRUE;
						fprintf(stderr,"%s -> trovata la directory %s \n",__func__,fdb->fcb.name);
						rec_remove(d,fdb,inode,0);				
						fprintf(stderr, "SimpleFS_remove() -> Libero l'inode %d\n", d->fdb->file_inodes[i]);
						ret=DiskDriver_freeInode(d->sfs->disk,d->fdb->file_inodes[i]);
						if(ret<0) {
							fprintf(stderr, "SimpleFS_remove() -> Errore durante liberazione inode %d\n", d->fdb->file_inodes[i]);
							return -1;
						}
						d->fdb->file_inodes[i]=-1;
						// Decremento il numero di file all'interno della cartella
						d->fdb->num_entries--;
						fprintf(stderr, "SimpleFS_remove() -> Libero il blocco data %d\n", inode->primoBlocco);
						ret = DiskDriver_freeBlock(d->sfs->disk, inode->primoBlocco);
						if (ret<0) return -1;
						//fprintf(stderr, "SimpleFs_remove() -> Libero il blocco data %d\n", n_block);
						//ret=DiskDriver_freeBlock(d->sfs->disk,n_block);	//cancella fdb o db perche ha percorso tutti i files_inodes[]
						//if(ret<0) return -1;
						ret = DiskDriver_writeBlock(d->sfs->disk, d->fdb, d->fdb->fcb.block_in_disk);
						if(ret<0) return -1;
						break;
						}
					}
				else{
					fprintf(stderr,"SimpleFS_remove() -> not recognized tipoFile");
					return -1;
					}
				//Libero l'inode
				//ret = DiskDriver_freeInode(d->sfs->disk, d->fdb->file_inodes[i]);
				//if ( ret < 0 ) return -1;
				//d->fdb->file_inodes[i]=-1;
			}
		}
		//int mancanti_lettura= fdbr->num_entries-limit;
		int mancanti_lettura= d->fdb->num_entries-limit;
		//n_block=fdbr->header.next_block;
		n_block=d->fdb->header.next_block;
		if(n_block<0) return -1;
		//passo ai num_db blocchi restanti
		int j;
		limit = (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int);
		for(i=0;i < num_db;i++){
			
			//ret=DiskDriver_readBlock(fs->disk,dbr,n_block);
			fprintf(stderr, "SimpleFS_remove() -> Leggo il DB a %d\n", n_block);
			ret=DiskDriver_readBlock(d->sfs->disk,dbr,n_block);
			if(ret<0) return -1;
			
			for(j=0;j< limit; j++){	//e poi leggo tutti i file_inodes[] dei blocchi successivi
				if(mancanti_lettura == 0) break;
				if ( dbr->file_inodes[j] != -1 )
				{
					//ret=DiskDriver_readInode(fs->disk,inode,dbr->file_inodes[j]);
					ret=DiskDriver_readInode(d->sfs->disk,inode,dbr->file_inodes[j]);
					if(dbr->file_inodes[j] < 0) continue;
					//ret=DiskDriver_readBlock(fs->disk,ffb,inode->primoBlocco);
					ret=DiskDriver_readBlock(d->sfs->disk,ffb,inode->primoBlocco);
					if(ret<0) return -1;
					if(inode->tipoFile=='r'){ //è un file
						//ret=DiskDriver_readBlock(fs->disk,ffb,inode->primoBlocco);	//mi basta il primo blocco quello con fcb
						ret=DiskDriver_readBlock(d->sfs->disk,ffb,inode->primoBlocco);	//mi basta il primo blocco quello con fcb
						if(strcmp(ffb->fcb.name,filename)==0){	//trovato il file e basta rimuoverlo
							found_file=TRUE;
							single_remove(d,ffb,inode);
							// libero l'inode
							fprintf(stderr, "SimpleFS_remove() -> DB: file- Libero l'inode %d\n", dbr->file_inodes[j]);
							ret = DiskDriver_freeInode(d->sfs->disk, dbr->file_inodes[j]);
							if ( ret<0) {
								fprintf(stderr, "SimpleFS_remove() -> DB: file- errore durante la liberazione dell'inode%d\n", dbr->file_inodes[j]);
								return -1;
							}
							dbr->file_inodes[j]=-1;
							// libero il primo blocco
							fprintf(stderr, "SimpleFS_remove() -> DB: file- Libero il blocco data %d\n", inode->primoBlocco);
							ret = DiskDriver_freeBlock(d->sfs->disk, inode->primoBlocco);
							if ( ret<0){
								fprintf(stderr, "SimpleFS_remove() -> DB: file- errore durante la liberazione del blocco data%d\n", inode->primoBlocco);
							}
							// Decremento il numero di file all'interno della cartella
							d->fdb->num_entries--;
							// Scrittura di quest directoryblock
							ret = DiskDriver_writeBlock(d->sfs->disk, dbr, n_block);
							if(ret<0){
								//fprintf(stderr, "SimpleFS_remove() -> DB: file- errore durante la liberazione dell'inode%d\n", dbr->file_inodes[j]);
								return -1;
							}
							// Devo riscrivere anche il FDB perché ho modificato il numero di file all'interno
							ret = DiskDriver_writeBlock(d->sfs->disk, d->fdb, d->fdb->fcb.block_in_disk);
							if(ret<0){
								return -1;
							}
							break;
						}
						
					}
					else if (inode->tipoFile=='d'){	//è una dir
						
						//ret=DiskDriver_readBlock(fs->disk,fdb,inode->primoBlocco);
						ret=DiskDriver_readBlock(d->sfs->disk,fdb,inode->primoBlocco);
						if(ret<0) return -1;
						if(strcmp(fdb->fcb.name,filename)==0){	//trovata la dir con name filename , eliminaz. ricorsiva
							found_file=TRUE;
							rec_remove(d,fdb,inode,0);
							//fprintf(stderr, "SimpleFs_remove() -> DB: Libero il blocco data %d\n", n_block);
							//ret=DiskDriver_freeBlock(d->sfs->disk,n_block);	//cancella fdb o db perche ha percorso tutti i files_inodes[]
							//if(ret<0) return -1;
							
							fprintf(stderr, "SimpleFS_remove() -> DB: cartella '%s'- Libero l'inode %d\n", fdb->fcb.name,dbr->file_inodes[j]);
							ret=DiskDriver_freeInode(d->sfs->disk,dbr->file_inodes[j]);
							fprintf(stderr, "SimpleFS_remove() -> DB: cartella '%s' - Ho liberato l'inode %d\n", fdb->fcb.name, dbr->file_inodes[j]);
							if(ret<0){
								fprintf(stderr, "SimpleFS_remove() -> DB: cartella '%s' - Errore durante la liberazione dell'inode %d\n", fdb->fcb.name,dbr->file_inodes[j]);
								return -1;
							}
							dbr->file_inodes[j]=-1;
							fprintf(stderr, "SimpleFS_remove() -> DB: cartella '%s'- Libero il blocco data %d\n", fdb->fcb.name,inode->primoBlocco);
							ret = DiskDriver_freeBlock(d->sfs->disk, inode->primoBlocco);
							if(ret<0){
								fprintf(stderr, "SimpleFS_remove() -> DB: cartella '%s' - Errore durante la liberazione del blocco data %d\n", fdb->fcb.name, inode->primoBlocco);
								return -1;
							}
							// Decremento il numero di file all'interno della cartella
							d->fdb->num_entries--;
							ret = DiskDriver_writeBlock(d->sfs->disk, dbr, n_block);
							if(ret<0) {
								fprintf(stderr, "SimpleFS_remove() -> DB: cartella - errore durante la scrittura del directoryblock %d\n", n_block);
								return -1;
							}
							// Devo scrivere le modifiche del FDB (num_entries--)
							ret = DiskDriver_writeBlock(d->sfs->disk, d->fdb, d->fdb->fcb.block_in_disk);
							if(ret<0) {
								fprintf(stderr, "SimpleFS_remove() -> DB: cartella - errore durante la scrittura del first directoryblock %d\n", n_block);
								return -1;
							}
							break;
						}
					}
					else{
						perror("SimpleFS_remove() -> not recognized tipoFile");
						return -1;
						}
					mancanti_lettura--;
				}
			}
			n_block=dbr->header.next_block;
			//assert(n_block>=0);
				
		}
	}
	
	
	free(ffb);	
	free(fdb);
	free(dbr);
	//free(fdbr);
	free(inode);
	
	if(!found_file){
		fprintf(stderr,"SimpleFS_remove() -> file not found \n");
		return -1;
	}
	
	Inode* inodeAggiornamento = malloc(sizeof(Inode));
	if ( DiskDriver_readInode(d->sfs->disk, inodeAggiornamento, d->inode) != -1 )
	{
		inodeAggiornamento->dataUltimoAccesso = time(NULL);
		if ( DiskDriver_writeInode(d->sfs->disk, inodeAggiornamento, d->inode) != -1 )
		{
			// OK
		}
		else
		{
			fprintf(stderr, "SimpleFS_remove() -> errore durante la scrittura dell'aggiornamento data ultimo accesso dell'inode %d\n", d->inode);
		}
	}
	else
	{
		fprintf(stderr, "SimpleFS_remove() -> Errore durante lettura per l'aggiornamento data ultimo accesso dell'inode %d\n", d->inode);
	}
	free(inodeAggiornamento);

	fprintf(stderr,"%s -> eliminazione completata \n",__func__);
	DiskDriver_flush(d->sfs->disk);
	return 0;
}

