#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int na, char **va)
{
	DiskDriver* disk = NULL;
	SimpleFS* fileSystem = (SimpleFS*)malloc(sizeof(SimpleFS));
	DirectoryHandle* directoryAttuale;
	FileHandle* fileAperto = NULL;
	int comando = 1;
	char comandoStringa[200];
	int numeroBlocchi = -1;
	char** contenutoDirectory;
	int i;
	int dimensioneArray;
	int trovato;
	int numeroBytes;
	int scritti;
	char* buffer;
	// Menu
	// 99 per uscire dall'applicazione
	while ( comando != 99 )
	{
		if ( disk == NULL) printf("1: Open or create disk with filesystem\n");
		if ( disk != NULL) printf("2: Format the disk opened\n");
		if ( disk != NULL) printf("3: Create a new empty file\n");
		if ( disk != NULL) printf("4: Create a new directory\n");
		if ( disk != NULL) printf("5: Change directory\n");
		if ( disk != NULL) printf("6: Show the files in the current directory\n");
		if ( disk != NULL) printf("7: Delete a file or a directory\n");
		if ( disk != NULL) printf("8: Open an existent file\n");
		if ( fileAperto != NULL ) printf("9: Write something at the position \n");
		if ( fileAperto != NULL ) printf("10: Read something at the position \n");
		if ( fileAperto != NULL ) printf("11: Seek in file\n");
		if ( fileAperto != NULL ) printf("12: Close opened file\n");
		if ( disk != NULL) printf("30: List files in the current directory\n");
		printf("99: Quit\n");
		printf("\nEnter a value: ");
		scanf("%d", &comando);
		switch ( comando )
		{
			// Apertura/creazione disco con filesystem
			case 1:
				printf("Selected option number 1\n");
				printf("1: Insert file name: ");
				scanf("%s", comandoStringa);
				getchar();
				if ( access(comandoStringa, F_OK) != 0 )
				{
					printf("File doesn't exist\n");
					while ( numeroBlocchi < 23 )
					{
						printf("Select the number of blocks (hard-coded) > 23:  ");
						scanf("%d", &numeroBlocchi);
						getchar();
					}
				}
				else
				{
					printf("File exists\n");
				}

				disk = (DiskDriver*) malloc(sizeof(DiskDriver));
				DiskDriver_init(disk, comandoStringa, numeroBlocchi);
				fileSystem->disk = disk;
				if ( numeroBlocchi != -1 )
				{
					printf("Formatto\n");
					SimpleFS_format(fileSystem);
				}
				directoryAttuale = SimpleFS_init(fileSystem, disk);
				break;
					
			// Formattare un filesystem aperto
			case 2:
				if ( disk != NULL )
				{
					SimpleFS_format(fileSystem);
					disk=NULL;
				}
				else
				{
					printf("File not opened\n");
				}
				break;
					
			// Creazione nuovo file
			case 3:
				if ( directoryAttuale != NULL )
				{
					do
					{
						printf("Select the name of the new file: ");
						scanf("%180s", comandoStringa);
						getchar();
					} while ( strlen(comandoStringa) < 1 );
					
					if ( SimpleFS_createFile(directoryAttuale, comandoStringa) == NULL )
					{
						printf("Can't create the new file\n");
					}
				}
				else
				{
					printf("You aren't in any directory\n");
				}
				break;
			
			// Creazione nuova directory
			case 4:
				if ( directoryAttuale != NULL )
				{
					do
					{
						printf("Select the name of the new directory: ");
						scanf("%180s", comandoStringa);
						getchar();
					} while ( strlen(comandoStringa) < 1 );
					if ( SimpleFS_mkDir(directoryAttuale, comandoStringa) != -1 )
					{
						// OK
					}
					else
					{
						printf("Can't create the new directory\n");
					}
					
				}
				else
				{
					printf("You aren't in any directory\n");
				}
				break;
				
			// Cambia directory
			case 5:
				if ( directoryAttuale != NULL )
				{
					do
					{
						printf("Select the name of the next directory: ");
						scanf("%180s", comandoStringa);
						getchar();
					} while ( strlen(comandoStringa) < 1 );
					if ( SimpleFS_changeDir(directoryAttuale, comandoStringa) != -1 )
					{
						// OK
						//printf("inode attuale: %d\n", directoryAttuale->sfs->current_directory_inode);
					}
					else
					{
						printf("The selected directory doesn't exist\n");
					}
				}
				else
				{
					printf("You aren't in any directory\n");
				}
				break;
			
			// Stampa contenuto directory
			case 6:
				contenutoDirectory = (char**)malloc(directoryAttuale->fdb->num_entries*sizeof(char*));
				dimensioneArray = directoryAttuale->fdb->num_entries;
				i = 0;
				while ( i < dimensioneArray )
				{
					contenutoDirectory[i] = (char*)malloc(128*sizeof(char));
					i++;
				}
				printf("%d files in current directory: \n", dimensioneArray);
				SimpleFS_readDir(contenutoDirectory, directoryAttuale);
				// Stampa dei file contenuti nella directory
				i = 0;

				while ( i < dimensioneArray )
				{
					printf("%s\n", contenutoDirectory[i]);
					i++;
				}
				// Libero la memoria
				i = 0;
				while ( i < dimensioneArray )
				{
					free(contenutoDirectory[i]);
					i++;
				}
				free(contenutoDirectory);
				break;
				
			// Rimuovi file o cartella 
			case 7:
				if ( directoryAttuale != NULL )
				{
					do
					{
						printf("Select the name of the file or directory to remove: ");
						scanf("%180s", comandoStringa);
						getchar();
					} while ( strlen(comandoStringa) < 1 );
					if ( SimpleFS_remove(directoryAttuale, comandoStringa) != -1 )				
					{
						printf("%s removed successfully\n", comandoStringa);
					}
					else
					{
						printf("Can't remove %s\n", comandoStringa);
					}
				}
				else
				{
					printf("You aren't in a directory\n");
				}
				break;
			
			// Apri file
			case 8:
				if ( fileAperto == NULL )
				{
					do
					{
						printf("Select the name of the file to open: ");
						scanf("%180s", comandoStringa);
						getchar();
					} while ( strlen(comandoStringa) < 1 );
					fileAperto = SimpleFS_openFile(directoryAttuale, comandoStringa);
					if ( fileAperto != NULL )
					{
						printf("File %s opened successfully\n", comandoStringa);
					}
					else
					{
						printf("Can't open file %s\n", comandoStringa);
					}
				}
				else
				{
					printf("Close the already opened file before to open a new one\n");
				}
				break;
			
			// Scrivi su file le cose lette da standard input
			case 9:
				if ( fileAperto != NULL )
				{
					do
					{
						printf("How many bytes do you want to write at the current position? ");
						scanf("%d", &numeroBytes);
						getchar();
					} while(numeroBytes <= 0);
					buffer = (char*) malloc((numeroBytes+1)*sizeof(char));
					printf("Type what do you want to write: ");
					fgets(buffer, numeroBytes+1, stdin);
					scritti = SimpleFS_write(fileAperto, buffer, numeroBytes);
					if ( scritti != -1 )
					{
						printf("Wrote successfully %d on file %s\n", scritti, fileAperto->ffb->fcb.name);
					}
					else
					{
						printf("Error in writing on %s\n", fileAperto->ffb->fcb.name);
					}
				}
				else
				{
					printf("File not opened\n");
				}
				break;
				
			// Stampa su schermo le cose lette dal file (numero byte indicato dallo standard input)
			case 10:
				if ( fileAperto != NULL )
				{
					int num_bytes;
					do
					{
						printf("Chose how many bytes to read: ");
						scanf("%d", &num_bytes);
						getchar();
						printf("Reading %d bytes\n",num_bytes);
					} while ( num_bytes ==0 );
					
					char * toRead= malloc(sizeof(char) * num_bytes);
					int toReadProva[4];
					int readed=SimpleFS_read(fileAperto,toReadProva,num_bytes);
					if( readed != -1){
						printf("Read %d from %s: \n %s \n",readed,fileAperto->ffb->fcb.name,toRead);
					}
					else{
						printf("Err in read %s \n",fileAperto->ffb->fcb.name);
					}
					
					free(toRead);
				}
				else
				{
					printf("File not opened\n");
				}
				break;
				
			// Spostati alla posizione indicata nello standard input nel file
			case 11:
				if ( fileAperto != NULL )
				{
					int pos,bytes_moved;
					do
					{
						printf("Chose position in the file: ");
						scanf("%d", &pos);
						getchar();
					} while ( pos < 0 );
					
					bytes_moved= SimpleFS_seek(fileAperto,pos);
					if(bytes_moved!=-1)
						printf("Moved %d bytes in %s \n",bytes_moved,fileAperto->ffb->fcb.name );
					else
						printf("Could not seek in %s \n",fileAperto->ffb->fcb.name);
				}
				else
				{
					printf("File not opened\n");
				}
				break;
			
			// Chiudi file aperto
			case 12:
				if ( fileAperto != NULL )
				{
					//fileAperto = NULL;
					if( SimpleFS_close(fileAperto) != -1){
						printf("Closed file \n"); 
						fileAperto = NULL;
					}
					else{
						printf("Could not close file \n"); 
					}
				}
				else
				{
					printf("File not opened\n");
				}
				break;
			
			// lista colorata
			case 30:
				//printf("inode attuale list: %d\n", directoryAttuale->sfs->current_directory_inode);
				SimpleFS_listFiles(directoryAttuale->sfs);
				break;
			
			// Dati disco
			case 50:
				if ( disk != NULL )
				{					
					printf("Header disk: \n");
					printf("Num total blocks: %d\n", disk->header->num_blocks);
					printf("Num data blocks on bitmap: %d\n", disk->header->bitmap_blocks);
					printf("Num inode on inode: %d\n", disk->header->inodemap_blocks);
					printf("Num data blocks free: %d\n", disk->header->dataFree_blocks);
					printf("Num inode free: %d\n", disk->header->inodeFree_blocks);
					//printf("Num total blocks: %d", disk->header->num_blocks);
				}
				else
				{
					printf("Open a disk please\n");
				}
				break;
			// Esci dall'applicazione
			case 99:
				printf("Goodbye!\n");
				// Rilascio le risorse
				if ( directoryAttuale != NULL )
				{
					close(directoryAttuale->sfs->disk->fd);
				}
				break;
				
			// Altri valori
			default:
				printf("Value not valid: %s\n", comandoStringa);
				break;
		}
		printf("\n\n");
		getchar();
	}
}
