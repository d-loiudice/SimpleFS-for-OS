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
	int comando = 1;
	char comandoStringa[200];
	int numeroBlocchi = -1;
	// Menu
	// 99 per uscire dall'applicazione
	while ( comando != 99 )
	{
		if ( disk == NULL) printf("1: Open or create disk with filesystem\n");
		printf("2: Format the disk opened\n");
		printf("3: Create a new file\n");
		printf("4: Create a new directory\n");
		printf("5: Change directory\n");
		printf("6: \n");
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
				break;
			
			case 4:
				break;
				
			case 5:
				break;
			
			case 6:
				break;
				
			case 99:
				printf("Goodbye!\n");
				break;
				
			default:
				printf("Value not valid: %s\n", comandoStringa);
				break;
		}
		getchar();
	}
}
