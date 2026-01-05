#ifndef ADD_H
#define ADD_H

#include <stdio.h>
#include <stdint.h>

#define FILE_PERMS 0755 // Owner can read/write/exec, Others can read/exed

typedef struct indexHeader
{
	char signature[4]; // DIRC
	uint32_t version;
	uint32_t entry_count;
} indexHeader;

typedef struct indexEntry
{
	uint32_t mode; // this will just be FILE_PERMS
	unsigned char sha1[20];
	uint16_t pathLen;
	uint32_t file_size;
	char* path;
} indexEntry;


/*
* @brief	initialize .tit/index with just a header and a checksum, no entries
*
* @return	0 if successful, -1 if failed
*
*/
int initIndex(void);

/*
* @brief	read the index, and store the entries into the indexEntry** entries array, and entry count
*
* @param	entries - pointer to array of pointers of indexEntries, will be allocated and populated by readIndex function
*
* @param	count - pointer to size_t entry counter, will be populated by readIndex function
*
* @return	0 if successful, -1 if failed
*/
int readIndex(struct indexEntry** entries, size_t* count);

/*
* @brief	free the entries array and count 
*
* @param	entries - array of indexEntries that will be freed
*
* @param	count - entry counter to be freed
*
* @return	0 if successful, -1 if failed
*
*/
int freeEntriesArr(struct indexEntry** entries, size_t count);

/*
* @brief	adds a file to the index/staging area
*
* @param	file - file name to be added
*
* @param	entries - array of indexEntries used to add files
* 
* @param	count - pointer to size_t entry counter, used under the hood
*
* @return	0 if successful, -1 if failed
*/
int addFile(char* file, struct indexEntry** entries, size_t* count);

#endif
