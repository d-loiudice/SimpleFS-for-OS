#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int na, char **va)
{
	DiskDriver* disk = NULL;
	SimpleFS* fileSystem = NULL;
	int comando = 1;
	char comandoStringa[200];
	// Menu
	// 99 per uscire dall'applicazione
	while ( comando != 99 )
	{
		printf("1: Open an existent disk with filesystem\n");
		printf("2: Create a new disk with filesystem\n");
		
		printf("Enter a value: ");
		scanf("%d", &comando);
		switch ( comando )
		{
			case 1:
				printf("Selected option number 1\n");
				break;
					
			case 2:
				printf("Selected option number 2\n");
				break;
				
			default:
				printf("Value not valid: %s\n", comandoStringa);
				break;
		}
		getchar();
	}
}
