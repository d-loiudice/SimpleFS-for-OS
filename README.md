# SimpleFS-for-OS

Academic project made by three people in order to build a simple virtual file system.

#Istruction :
to compile, in shell exec:
$ make so_game

to launch the program:
TODO

# Mimimum requirements for this academic project
 
 File System 
   implement a file system interface using binary files
   - The file system reserves the first part of the file
     to store:
     - a linked list of free blocks
     - linked lists of file blocks
     - a single global directory
     
   - Blocks are of two types
     - data blocks
     - directory blocks

     A data block are "random" information
     A directory block contains a sequence of
     structs of type "directory_entry",
     containing the blocks where the files in that folder start
     and if they are directory themselves
     
     
     
     

[||||]

Ad esempio 30 blocchi totali allocabili (grandezza della partizione)


Abbiamo:
1 blocco superblocco
1 blocco bitmap
1 blocco inodemap
4 blocchi di inode (quindi 16 * 4 file/directory associate)
23 data block  [bitmap_blocks]
