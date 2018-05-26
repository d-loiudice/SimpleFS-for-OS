#include "simplefs.h"
#include <stdio.h>
#define DEBUG 1




int main(int agc, char** argv) {
  //#if DEBUG
	  // prova di linking TODO da rimuovere 
	  printf("INIZIO DEBUGGING FUNZIONI...\n");
	  
	  printf("----| diskDriver MODULE |----: \n");
	  printf("\t init: void\n"); DiskDriver_init(NULL,"fileTest.txt",2);
	  //printf("read: %d\n",DiskDriver_readBlock(NULL,NULL,1) );
	  
	  printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
	  printf("DataBlock size %ld\n", sizeof(FileBlock));
	  printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
	  printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));
	  
 // #endif
}
