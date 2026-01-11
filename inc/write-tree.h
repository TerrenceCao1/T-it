#ifndef WRITE_TREE_H
#define WRITE_TREE_H

#include <stdint.h>
#include <stdio.h>

/*
* @brief	writeTree converts the current index into a tree object, hashes, and puts it into object folder
*
* @param	write - bool on whether to write it or not
*
* @return	hash of tree
*/
uint8_t* writeTree(_Bool write);

/*
* @brief	compresses a file using zlib and puts it into the fileOut
*
* @param	fileIn - string with the path to the input file
*
* @param	fileOut - string with the path to the output file
*
* @return	0 if success, 1 if not
*/
int compressFile(char* fileIn, char* fileOut);
#endif
