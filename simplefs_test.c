#include "simplefs.h"
#include <stdio.h>
int main(int agc, char** argv) {
  // prova di linking TODO da rimuovere 
  printf("----> %d\n",DiskDriver_readBlock(NULL,NULL,1) );
  
  printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
  printf("DataBlock size %ld\n", sizeof(FileBlock));
  printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
  printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));
  
}
