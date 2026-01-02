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



int initIndex(void);

int readIndex(struct indexEntry** entries, size_t* count);


#endif
