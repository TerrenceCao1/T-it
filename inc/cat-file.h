#ifndef CAT_FILE_H
#define CAT_FILE_H

#include <stdint.h>

#define FILE_PERMS 0755 // Owner can read/write/exec, Others can read/exed

#define TRUE 1
#define FALSE 0

#define CHUNK 16384 // ZLIB CHUNK SIZE
/*
 * @brief	prints out contents of a tit object to to stdout
 *
 * @param	hash - the hash of the tit object 
 *
 * @param	type - flag for if we need to print type
 *
 * @param	size - flag for if we need to print size
 * 
 * @param	blob - flag for if we need to print blob contents
 *
 * return	0 if successful, -1 if error
 *
 *
 * */
int catFile(char* hash, _Bool type, _Bool size, _Bool blob);

#endif
