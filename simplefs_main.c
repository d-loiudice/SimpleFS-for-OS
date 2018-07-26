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
		if ( disk != NULL) printf("7: Open an existent file\n");
		if ( fileAperto != NULL ) printf("8: Write something at the position \n");
		if ( fileAperto != NULL ) printf("9: Read something at the position \n");
		if ( fileAperto != NULL ) printf("10: Seek in file\n");
		printf("99: Quit\n");
		printf("\nEnter a value: ");
		scanf("%d", &comando);
		switch ( comando )
		{
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
									
			case 2:
				if ( disk != NULL )
				{
					SimpleFS_format(fileSystem);
				}
				else
				{
					printf("File not opened\n");
				}
				break;
							
			case 3:
				if ( directoryAttuale != NULL )
				{
					do
					{
						printf("Select the name of the new file: ");
						scanf("%180s", comandoStringa);
						getchar();
					} while ( strlen(comandoStringa) < 1 );
					fileAperto = SimpleFS_createFile(directoryAttuale, comandoStringa);
					if ( fileAperto == NULL )
					{
						printf("Can't create the new file\n");
					}
				}
				else
				{
					printf("You aren't in any directory\n");
				}
				break;
			
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
				
			case 5:
				if ( directoryAttuale != NULL )
				{
					do
					{
						printf("Select the name of the new directory: ");
						scanf("%180s", comandoStringa);
						getchar();
					} while ( strlen(comandoStringa) < 1 );
					if ( SimpleFS_changeDir(directoryAttuale, comandoStringa) != -1 )
					{
						// OK
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
				
			case 99:
				printf("Goodbye!\n");
				// Rilascio le risorse
				if ( directoryAttuale != NULL )
				{
					close(directoryAttuale->sfs->disk->fd);
				}
				break;
				
			default:
				printf("Value not valid: %s\n", comandoStringa);
				break;
		}
		printf("\n\n");
		getchar();
	}
}
