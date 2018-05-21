#include <stdio.h>
#include <stdlib.h>
#include "inode_bitmap.h"

// Dato l'indice della InodeMap, ritorna l'oggetto InodeMapEntryKey corrispondente
InodeMapEntryKey InodeMap_getFromIndex(int indice){
	InodeMapEntryKey* imek = (InodeMapEntryKey*) malloc(sizeof(InodeMapEntryKey));
	imek->indice=indice/8;
	imek->inode=NULL;
	imek->bit_num=indice % 8;
	return *imek;
}

// Data l'inodemap e una posizione di partenza, trova il primo indice corrispondente allo stato status ( 0 libero, 1 occupata )
int InodeMap_get(InodeMap* inodemap, int start, unsigned char status){
	int i;
	for(i=start;i < inodemap->numeroInodes;i++){
		if(inodemap->mappa[i]==status)
			return i;
	}
	return -1;
}

// Data l'inodemap, imposta lo stato status (0 / 1) alla posizione pos
int InodeMap_set(InodeMap* inodemap, int pos, unsigned char status){
	if(pos < 0 || pos >= inodemap->numeroInodes){
		perror("ERR: pos in inodemap non valida");
		return -1;
	}
	inodemap->mappa[pos]=status;
	return 0;
}

