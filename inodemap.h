#pragma once
// Libreria per accesso standardizzato alle funzioni di acquisizione e manipolazione del tempo it.wikipedia.org/wiki/Time.h
#include <time.h>

typedef struct 
{
	// Numero totali di inodes
	unsigned int numeroInodes;
	// Mappa degli inodes liberi o occupati
	unsigned char* mappa;
} InodeMap;

// Dimensione totale della struttura = 1+1+1+1+1+4+4+4+4+1+(4*12)+4+4+4 = 82 -> sarebbe bello che fosse un divisore di 512, almeno non si spreca spazio nei blocchi
typedef struct
{
	// unsigned char == uint8_t, per memorizzare byte / piccoli valori ( sizeof(unsigned char) = 1 )
	// Permessi dell'utente a cui appartiene il file, valori possibili da 0 a 7 (rwx)
	unsigned char permessiUtente;
	// Permessi del gruppo a cui appartiene il file, valori possibili da 0 a (rwx)
	unsigned char permessiGruppo;
	// Permessi agli altri (rwx) da 0 a 7
	unsigned char permessiAltri;
	// La lista degli utenti si potrebbe registrare su un file nel filesystem ( es. users.pippo )
	// Id dell'utente a cui appartiene il file
	unsigned char idUtente;
	// Id del gruppo 
	unsigned char idGruppo;
	// Data creazione del file
	time_t dataCreazione;
	// Data ultima modifica del file
	time_t dataUltimaModifica;
	// Dimensione del file ( in bytes ) massimo valore 4GB più o meno essendo long int = 32bit
	unsigned long int dimensioneFile;
	// Dimensione in blocchi (quanti blocchi occupa il file descritto da questo inode)
	unsigned int dimensioneBlocchi;
	// Tipo di file ( r = file regolare, d = directory, ecc... )
	char tipoFile;
	
	// Gestione dell'indirizzamento ai blocchi contenente il contenuto del file
	// Per ora massima dimensione massima file possibile con questi puntatori = 1GB ( circa 1 082 202 112 bytes )
	// en.wikipedia.org/wiki/Inode_pointer_structure
	// Sulle slide è Indexed Allocation, del pdf File System Implementation
	// Se settato a -1, non punta da nessuna parte 
	// Puntatore ai blocchi diretti 12: contengono l'indice del blocco contenente il contenuto del file
	// Possibiltà di accedere ai primi 12*(BLOCK_SIZE - HEADER_SIZE) del file 
	// [] -> [contenuto del file]
	int[12] puntatoriDiretti;
	
	// Puntatore indiritto ai blocchi: contiene l'indice del blocco contenente una lista di indici dei blocchi contenente 
	// il contenuto del file
	// [] -> [ | ], dove ogni quadratino punta al contenuto del file
	int puntatoreIndiretto;
	
	// Puntatore doppiamente indiretto: contiene l'indice del blocco contenente una lista di indici dei blocchi, dove ciascun
	// blocco contiene a sua volta una lista di indici dei blocchi contenente il contenuto del file
	// [] -> [ | ], dove ogni quadratino -> [ | | ], infine questi puntano al blocco dove c'è il contenuto del file
	int puntatoreDoppioIndiretto;
	
	// Puntatore triplamente indiretto: contiene l'indice del blocco contenente una lista di indici dei blocchi, dove ciascun
	// blocco contiene a sua volta una lista di indici dei blocchi con altri indici dei blocchi dove questi ultimi hanno finalmente
	// gli indici dei blocchi contenente il contenuto del file
	// [] -> [ | ], dove ogni quadratino -> [ | | | ], ciascun quadratino punta ad un'altra lista di quadratini [ | | | | ] nella quale
	// ogni quadratino punta ad un blocco contenente il contenuto del file
	int puntatoreTriploIndiretto;
} Inode;

typedef struct 
{
	// Indice dell'inode nella mappa
	int indice;
	// La struttura inode
	Inode* inode;
} InodeMapEntryKey;

// Dato l'indice della InodeMap, ritorna l'oggetto InodeMapEntryKey corrispondente
InodeMapEntryKey InodeMap_getFromIndex(int indice);

// Data l'inodemap e una posizione di partenza, trova il primo indice corrispondente allo stato status ( 0 libero, 1 occupata )
int InodeMap_get(InodeMap* inodemap, int start, unsigned char status)

// Data l'inodemap, imposta lo stato status (0 / 1) alla posizione pos
int InodeMap_set(InodeMap* inodemap, int pos, unsigned char status)



