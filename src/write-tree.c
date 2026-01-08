#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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
#include "write-tree.h"

// Used for sorting our index
static int cmp_entries(const void *a, const void *b)
{
	const indexEntry *ea = a;
	const indexEntry *eb = b;
	return strcmp(ea->path, eb->path);
}

int writeTree(void)
{
	// get our entries
	struct indexEntry* entries;
	size_t count;
	readIndex(&entries, &count);

	// must be sorted
	qsort(entries, count, sizeof(entries[0]), cmp_entries);

	// write relevant info to a temp file
	FILE* fp = fopen(".tit/temp/treeWriter", "wb");
	if(!fp) return -1;

	// compute entries size
	size_t entriesSize = 0;
	for(int i = 0; i < count; i++)
	{
		entriesSize += 4 + 1; // mode is 4 bytes + 1 for space
		entriesSize += entries[i].pathLen + 1; // path and null
		entriesSize += 20; // SHA1
	}

	// for header
	fprintf(fp, "tree %zu", entriesSize);
	fputc('\0', fp);

	for(int i = 0; i < count; i++)
	{
		fprintf(fp, "%u ", entries[i].mode);
		fprintf(fp, "%s", entries[i].path);
		fputc('\0', fp);
		fprintf(fp, "%s", entries[i].sha1);
	}

	fclose(fp);
	freeEntriesArr(&entries, count);

	// GET SIZE AND MAKE HASH!
	// entries + 5 for "tree " + length of entriesSize
	size_t treeSize = entriesSize + 5 + floor(log10(entriesSize));
	uint8_t* hash = hashTree(treeSize);
	if(hash == NULL) return -1;

	// make dir
	char dir[3];
	sprintf(dir, "%02x", hash[0]);

	char finalDir[100] = ".tit/objects/";
	strcat(finalDir, dir);
	strcat(finalDir, "/");

	if(mkdir(finalDir, FILE_PERMS) == 0)
	{
		char fileName[SHA_DIGEST_LENGTH * 2] = "";
		char temp[3];
		for(int i = 1; i < SHA_DIGEST_LENGTH; i++)
		{
			sprintf(temp, "%02x", hash[i]);
			strcat(fileName, temp);
		}

		// append our filename to finalDir
		strcat(finalDir, fileName);
		FILE* fp = fopen(finalDir, "wb");
		if(fp == NULL)
		{
			perror(fileName);
			return -1;
		}
		// printf("directory made: %s\n", finalDir);

		if(compressFile(".tit/temp/treeWriter", finalDir) != Z_OK)
		{
			return -1;
		}
	}
	free(hash);
	return 0;
}

uint8_t* hashTree(size_t treeSize)
{
	// put all file contents into a buffer for hashing 
	uint8_t* hash = (uint8_t*)calloc(SHA_DIGEST_LENGTH, sizeof(uint8_t));
	uint8_t buffer[treeSize];

	FILE* fp = fopen(".tit/temp/treeWriter", "rb");
	if(!fp) return NULL;

	// populate buffer
	fread(buffer, sizeof(buffer[0]), treeSize, fp);
	fclose(fp);

	SHA1(buffer, treeSize, hash);

	return hash;
}

int compressFile(char* fileIn, char* fileOut)
{
	FILE* inFile = fopen(fileIn, "rb");
	FILE* outFile = fopen(fileOut, "wb");

	// if any are NULL
	if(!inFile || !outFile)
	{
		perror("fopen");
		fclose(inFile);
		fclose(outFile);
		return -1;
	}

	int ret, flush;
	unsigned have;
	z_stream strm;
	uint8_t inBuffer[CHUNK];
	uint8_t outBuffer[CHUNK];

	// set z_stream defaults and initiate deflate:
	memset(&strm, 0, sizeof(strm));

	ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
	if(ret != Z_OK)
	{
		return ret;
	}

	// compress till the end of file
	do
	{
		// read a CHUNK of the input file and store into input buffer
		strm.avail_in = fread(inBuffer, 1, CHUNK, inFile);
		if(ferror(inFile)) // if this john messes up
		{
			(void)deflateEnd(&strm);
			return Z_ERRNO;
		}

		// check if it's the end of the file, if not keep going
		flush = feof(inFile) ? Z_FINISH : Z_NO_FLUSH;
		strm.next_in = inBuffer;

		do
		{
			strm.avail_out = CHUNK;
			strm.next_out = outBuffer;

			// deflate that chunk!
			ret = deflate(&strm, flush);
			assert(ret != Z_STREAM_ERROR);

			have = CHUNK - strm.avail_out;

			// write the outBuffer to the outFile
			if(fwrite(outBuffer, 1, have, outFile) != have || ferror(outFile))
			{
				(void)deflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);
		assert(strm.avail_in == 0); // check all input is used

	} while (flush != Z_FINISH);
	assert(ret == Z_STREAM_END);

	(void)deflateEnd(&strm);
	fclose(inFile);
	fclose(outFile);

	return Z_OK;
}
