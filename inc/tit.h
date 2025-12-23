/*
 * @file	Header file for all of the main T-it functions!
 *
 * @author	Terrence Cao (TCow-Kachow)
 *
 * @date	Dec 16 2025
 *
 * */

#ifndef TIT_H
#define TIT_H

#include <stdio.h>
#include <stdint.h>

#define FILE_PERMS 0755 // Owner can read/write/exec, Others can read/exed

typedef enum
{
	BLOB,
	TREE,
	COMMIT,
} OBJECT_TYPE;

/*
 * @brief	initializes a tit repo in the specified path
 *			
 *			makes the necessary directories: .tit, .tit/objects, .tit/refs, .tit/refs/heads, .tit/HEAD
 *
 * @param	path - string of the path that we are workin in
 *
 * @return void 
 *
 * */

int init(const char* path);


/*
 * @brief	creates the SHA1 hash of a file (a blob)
 *
 * @param	file - file you want to be hashed
 *
 * return	pointer to buffer containing the hash
 * */

uint8_t* hashBlob(char* file);

/* 
 * @brief	compress a file into an outfile
 *
 * @param	fileIn - input file
 *
 * @param	fileOut - output .z file (compressed)
 *
 * return	void
 * */
int compressBlob(char* fileIn, char* fileOut);

void test_hash(OBJECT_TYPE type, char* file);

#endif
