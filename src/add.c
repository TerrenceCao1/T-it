#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>
#include <dirent.h>
#include <openssl/sha.h>
#include <assert.h>
#include <arpa/inet.h>
#include <zlib.h>
#include "add.h"
#include "hash-object.h"

// TIT ADD and Helper Functions:
/* tit add will:
 * write an object file for the file added
 * create an indexEntry - with mode, sha1 hash, path, file_size
 * write the indexEntry to the .tit/index file
 */

static size_t getFileSize(char* file)
{
	FILE* fp = fopen(file, "rb");
	fseek(fp, 0L, SEEK_END);
	return ftell(fp);
}

// this function initiates an index header (caller must free!), and makes the index file
// There is only ONE index header (that is global and updates whenever an entry is added/rmed)
static int writeIndexHeader(uint32_t entryCount)
{
	// creating the index file
	remove(".tit/index");
	FILE* fp = fopen(".tit/index", "ab");
	if(fp == NULL)
	{
		perror(".tit/index");
		return -1;
	}

	uint32_t version = htonl(2);
	entryCount = htonl(entryCount);
	fwrite("DIRC", 1, 4, fp);
	fwrite(&version, 4, 1, fp);
	fwrite(&entryCount, 4, 1, fp);

	fclose(fp);
	return 0;
}

// TODO: make writeChecksum not take into account the previous checksum 
static int writeChecksum(void)
{
	size_t size = getFileSize(".tit/index");
	FILE* fp = fopen(".tit/index", "ab+");
	if(fp == NULL)
	{
		perror(".tit/index");
		return -1;
	}

	unsigned char shaInputData[size];
	unsigned char shaOutputData[SHA_DIGEST_LENGTH];

	fread(shaInputData, size, 1, fp);
	SHA1(shaInputData, size, shaOutputData);

	fwrite(shaOutputData, SHA_DIGEST_LENGTH, 1, fp);

	fclose(fp);
	return 0;
}

int initIndex(void)
{
	if(writeIndexHeader(0) == -1) return -1;
	writeChecksum();

	return 0;
}

// create an indexEntry from a path to an object
indexEntry* createIndexEntry(char* filePath)
{
	// make the object file
	if(writeObject(filePath, NULL) == -1)
	{
		return NULL;
	}

	// initialize an indexEntry, with the stuff from the file we have
	indexEntry* entry = malloc(sizeof(struct indexEntry));
	if(entry == NULL) return NULL;

	entry->mode = FILE_PERMS;
	entry->pathLen = strlen(filePath);
	entry->path = strdup(filePath);
	entry->file_size = getFileSize(filePath);

	uint8_t* sha1 = hashBlob(filePath, FALSE);
	memcpy(entry->sha1, sha1, SHA_DIGEST_LENGTH);

	return entry;
}

// readIndex reads the file at .tit/index and outputs an array of entries in the file, and the count of them.

/*
 * ENTRY FORMAT:
 * 32-bit mode (KEEP)
 * 32-bit file size (KEEP)
 * 20 BYTE SHA1 hash
 * 16-bit filePath
 * variable-len PathName
 */
int readIndex(struct indexEntry** entries, size_t* count)
{
	if(!entries || !count)
	{
		return -1;
	}
	*entries = NULL;
	*count = 0;

	FILE* fp = fopen(".tit/index", "rb");
	if(!fp) return 0; // empty index is ok also

	long fileSize = getFileSize(".tit/index");

	// read and validate the header:
	indexHeader hdr;
	fread(&hdr, 1, 12, fp);

	if(memcmp(hdr.signature, "DIRC", 4) != 0)
	{
		printf("INVALID INDEX SIGNATURE!\n");
		return -1;
	}

	uint32_t version = ntohl(hdr.version);
	if(version != 2)
	{
		printf("Unsupported version!\n");
		return -1;
	}

	*count = ntohl(hdr.entry_count);

	// make the entries array
	*entries = calloc(*count, sizeof(struct indexEntry));
	if(!*entries) return -1;

	// iterate through each entry
	for(int i = 0; i < *count; i++)
	{
		// just consume ctime sec, ctime nano, mtime sec, mtime nano, dev, ino cause we don't care lol
		uint32_t temp;

		// read mode
		fread(&temp, 4, 1, fp);
		(*entries)[i].mode = temp;

		// read file size!
		fread(&temp, 4, 1, fp);
		(*entries)[i].file_size = temp;

		// read SHA1 hash!
		fread((*entries)[i].sha1, 20, 1, fp);

		// get the pathLen from the flags
		fread(&temp, 2, 1, fp);
		(*entries)[i].pathLen = temp;

		// KEEP PATH!
		uint16_t pathLen = temp;
		(*entries)[i].path = malloc((pathLen + 1) * sizeof(char));
		fread((*entries)[i].path, pathLen, 1, fp);
		(*entries)[i].path[pathLen] = '\0';

		// Consume and check that next char is null
		char nul;
		fread(&nul, 1, 1, fp);

		if(nul != '\0')
		{
			return -1;
		}

		// Consume the padding - with formula (8 - (entryLen % 8) % 8)
		int entryLen = 30 + pathLen + 1;
		int padding = (8 - (entryLen % 8)) % 8;

		fseek(fp, padding, SEEK_CUR);
	}

	long pos = ftell(fp);
	if((fileSize - pos) != SHA_DIGEST_LENGTH)
	{
		printf("Error - checksum length is incorrect\n");
		return -1;
	}

	fclose(fp);
	return 0;
}

int freeEntriesArr(struct indexEntry** entries, size_t count)
{
	if(!entries || !*entries) return -1;
	if(count == 0) return 0;

	for(int i = 0; i < count; i++)
	{
		free((*entries)[i].path);
	}
	free(*entries);
	return 0;
}

int addEntryToIndex(struct indexEntry** entries, size_t* count, indexEntry entry)
{
	readIndex(entries, count);

	// iterate through list and see if there's a replacement
	for(int i = 0; i < *count; i++)
	{
		if(strcmp((*entries)[i].path, entry.path) == 0)
		{
			free((*entries)[i].path);
			(*entries)[i] = entry;
			return 0;
		}
	}

	struct indexEntry *temp = realloc(*entries, (*count + 1) * sizeof(struct indexEntry));
	if (!temp) return -1;

	// append it on WE WILL SORT LATER!
	*entries = temp;
	(*entries)[*count] = entry;
	(*count)++;

	return 0;
}

// Comparing entries for qsort
static int cmp_entries(const void *a, const void *b)
{
	const indexEntry *ea = a;
	const indexEntry *eb = b;
	return strcmp(ea->path, eb->path);
}

/*
 * ENTRY FORMAT:
 * 32-bit mode (KEEP)
 * 32-bit file size (KEEP)
 * 20 BYTE SHA1 hash
 * 16-bit pathLen
 * variable-len PathName
 */
int writeIndex(struct indexEntry** entries, size_t* count)
{
	if(writeIndexHeader(*count) == -1)
	{
		return -1;
	}

	// sort the entires so we have alphabetical order
	qsort(*entries, *count, sizeof(indexEntry), cmp_entries);

	FILE* fp = fopen(".tit/index", "ab");
	if(fp == NULL)
	{
		return -1;
	}

	// write the entries to .tit/index
	for(int i = 0; i < *count; i++)
	{
		fwrite(&((*entries)[i].mode), 4, 1, fp);
		fwrite(&((*entries)[i].file_size), 4, 1, fp);
		fwrite(&((*entries)[i].sha1), 20, 1, fp);
		fwrite(&((*entries)[i].pathLen), 2, 1, fp);
		fwrite((*entries)[i].path, (*entries)[i].pathLen + 1, 1, fp);

		int entryLen = 30 + (*entries)[i].pathLen + 1;
		int padding = (8 - (entryLen % 8)) % 8;
		for(int p = 0; p < padding; p++)
		{
			fputc('\0', fp);
		}
	}
	fclose(fp);

	if(writeChecksum() == -1)
	{
		return -1;
	}

	return 0;
}

int addFile(char* file, struct indexEntry** entries, size_t* count)
{
	struct indexEntry* entry = createIndexEntry(file); 
	if(entry == NULL)
	{
		printf("Invalid File.\n");
		return -1;
	}
	if(addEntryToIndex(entries, count, *entry) == -1)
	{
		return -1;
	}
	if(writeIndex(entries, count) == -1)
	{
		return -1;
	}

	return 0;
}
