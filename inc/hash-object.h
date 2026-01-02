#ifndef HASH_OBJECT_H
#define HASH_OBJECT_H

#include <stdio.h>
#include <stdint.h>

#define FILE_PERMS 0755 // Owner can read/write/exec, Others can read/exed

#define TRUE 1
#define FALSE 0

#define CHUNK 16384 // ZLIB CHUNK SIZE

typedef enum
{
	BLOB,
	TREE,
	COMMIT,
} OBJECT_TYPE;


/*
 * @brief	creates the SHA1 hash of a file (a blob)
 *
 * @param	file - file you want to be hashed
 *
 * return	pointer to buffer containing the hash
 * */

uint8_t* hashBlob(char* file, _Bool write);

/* 
 * @brief	compress a file into an outfile
 *
 * @param	dataBuffer - buffer containing the data that we're gonna compress
 *
 * @param	dataLen - length of data that's gonna be compressed.
 *
 * @param	fileOut - output .z file (compressed)
 *
 * return	void
 * */
int compressBlobBuffer(uint8_t* dataBuffer, size_t dataLen, char* fileOut);

/*
 * @brief	writing a file to .tit/objects
 *
 * @param	file - string w file name
 *
 * @parm	finalDirOut - optional argument if we want to return the final directory out
 *
 * @return	0 if it works -1 if it doesn't
 *
 */

int writeObject(char* file, char* finalDirOut);
#endif
