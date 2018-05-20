// Dato l'indice della InodeMap, ritorna l'oggetto InodeMapEntryKey corrispondente
InodeMapEntryKey InodeMap_getFromIndex(int indice);

// Data l'inodemap e una posizione di partenza, trova il primo indice corrispondente allo stato status ( 0 libero, 1 occupata )
int InodeMap_get(InodeMap* inodemap, int start, unsigned char status)

// Data l'inodemap, imposta lo stato status (0 / 1) alla posizione pos
int InodeMap_set(InodeMap* inodemap, int pos, unsigned char status)



