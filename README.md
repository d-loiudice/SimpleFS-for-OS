# SimpleFS-for-OS

Academic project made by three people in order to build a simple virtual file system.

## Instructions :

To compile:
$ make simple_fs 
to run the control interface (inside bash, no gui provided):
$ ./simple_fs

## Modules description:
- bitmap.h -> all the functions about the two bitmaps
- disk_driver.h -> functions about block memory of our File System 
- simplefs.h -> functions and data to manage directory and files at high level:
	create,read,list,remove them
- inode.h in ->  data structure for the inode metadata


## Project requirements:
 
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
     
     
Our project is based on a simple File System implementation, 
which is a simplified version of a commercial File System  in use today and 
this means that it is possible to creare Directory and File and access them 
(list,read,write,delete). To do that we divided disk space in fixed size 
(512 B) blocks:

[S|B|B|I|I|I|I|I|D|D|D|D|D|D...]


- First block (S) is used to store DiskHeader structure which contains
general info on File System
- Second one (B) is used to store the Data Bitmap (see below)
- Third one (B) is used to store the Inode Bitmap 
- Next five blocks (I) are used to store inodes, wich contains metadata upon 
file and directories stored in D
- Last blocks (D) are used to  store file and directories
 
 
## About the BitMap concept
Bitmap are data structures implemented with arrays of char in wich each bit
of a char entry, show if the block in disk with the index of 
the block + the microindex of the bit is free (0 value) or allocated (1 value).
We use two instances of them one for data blocks (Data Bitmap) and one for 
Inode blocks (Inode Bitmap).
 

## Credits:

Project made by three cs students :
 Tiziano Bari -> https://github.com/RareAverage301
 Giuseppe Capaldi -> https://github.com/N0t-A-G3n1us
 Davide Lo Iudice -> https://github.com/LinguaggioScalabile
