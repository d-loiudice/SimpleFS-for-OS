#pragma once
// Libreria per accesso standardizzato alle funzioni di acquisizione e manipolazione del tempo 
#include <time.h>
// Dimensione totale della struttura = 1+1+1+1+1+4+4+4+4+1+4 = 30+2padding -> sarebbe bello che fosse un divisore di 512, almeno non si spreca spazio nei blocchi
// no maths here: sizeof(Inode) senza padding = 54
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
	// Data ultimo accesso
	time_t dataUltimoAccesso;
	// Data ultima modifica del file
	time_t dataUltimaModifica;
	// Dimensione del file ( in bytes ) massimo valore 4GB più o meno essendo long int = 32bit
	unsigned long int dimensioneFile;
	// Dimensione in blocchi (quanti blocchi occupa il file descritto da questo inode)
	unsigned int dimensioneInBlocchi;
	// Tipo di file ( 'r' = file regolare, 'd' = directory, ecc... )
	char tipoFile;
	// Indice del primo blocco del file (LINKED LIST BLOCKS)
	unsigned int primoBlocco;
	// Padding per raggiungere i 64byte
	char padding[10];
	/// NO NEED: non avevo lette le specifiche
	// Gestione dell'indirizzamento ai blocchi contenente il contenuto del file
	// Per ora massima dimensione del file possibile con questi puntatori = 1GB ( circa 1 082 202 112 bytes )
	// en.wikipedia.org/wiki/Inode_pointer_structure
	// Sulle slide è Indexed Allocation, del pdf File System Implementation
	// ----Se settato a -1, non punta da nessuna parte 
	// Puntatore ai blocchi diretti 12: contengono l'indice del blocco contenente il contenuto del file
	// Possibiltà di accedere ai primi 12*(BLOCK_SIZE - HEADER_SIZE) del file 
	// [] -> [contenuto del file]
	//int[12] puntatoriDiretti;
	
	// Puntatore indiritto ai blocchi: contiene l'indice del blocco contenente una lista di indici dei blocchi contenente 
	// il contenuto del file
	// [] -> [ | ], dove ogni quadratino punta al contenuto del file
	//int puntatoreIndiretto;
	
	// Puntatore doppiamente indiretto: contiene l'indice del blocco contenente una lista di indici dei blocchi, dove ciascun
	// blocco contiene a sua volta una lista di indici dei blocchi contenente il contenuto del file
	// [] -> [ | ], dove ogni quadratino -> [ | | ], infine questi puntano al blocco dove c'è il contenuto del file
	//int puntatoreDoppioIndiretto;
	
	// Puntatore triplamente indiretto: contiene l'indice del blocco contenente una lista di indici dei blocchi, dove ciascun
	// blocco contiene a sua volta una lista di indici dei blocchi con altri indici dei blocchi dove questi ultimi hanno finalmente
	// gli indici dei blocchi contenente il contenuto del file
	// [] -> [ | ], dove ogni quadratino -> [ | | | ], ciascun quadratino punta ad un'altra lista di quadratini [ | | | | ] nella quale
	// ogni quadratino punta ad un blocco contenente il contenuto del file
	//int puntatoreTriploIndiretto;
} Inode;
