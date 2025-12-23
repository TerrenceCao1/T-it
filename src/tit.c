#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>
#include <dirent.h>
#include <openssl/sha.h>
#include <assert.h>
#include <zlib.h>
#include "tit.h"

// CHUNK size for zlib
#define CHUNK 16384


// INIT FUNCTION STATICS + init function
static int mkdirAtPath(const char *path)
{
	//make the path 
	if(mkdir(path, FILE_PERMS) == 0) return 0;
	if(errno == EEXIST) return 0; //if the dir exists already, that's chill
	perror(path);
	
	return -1;
}

static int initialize_repo(const char* path)
{
	if(mkdirAtPath(".tit") != 0) return -1;
	if(mkdirAtPath(".tit/objects") != 0) return -1;
	if(mkdirAtPath(".tit/refs") != 0) return -1;
	if(mkdirAtPath(".tit/refs/heads") != 0) return -1;

	FILE *f = fopen(".tit/HEAD", "w");
	if(f == NULL)
	{
		perror(".tit/HEAD");
		return -1;
	}

	fprintf(f, "ref: refs/heads/main\n");
	fclose(f);

	return 0;
}

int init(const char* path)
{

	//ERROR CHECKS 
	DIR *dir = opendir(path);
	if(dir == NULL) // path doesn't exist
	{
		perror(path);
		return -1;
	}

	// path is empty
	struct dirent* entry;
	int n = 0;

	while((entry = readdir(dir)) != NULL)
	{
		if(++n >= 2) break;
	}
	closedir(dir);

	if(n > 2) // if dir is not empty because it contains more than just '.' and '..'
	{
		printf("dir not empty...");
		return -1;
	}

	if(initialize_repo(path) == 0)
	{
		printf("Initialized tit repo!\n");
	};
	return 0;
}

// HASH BLOB STATICS + function
/*
 * build full buffer (type size data)
 * hash it into raw digest
 * conv raw digest into hex
 * derive object path from the hex
 *
 * */

/*
 * This function builds the header in form: "<type> <size>"
 * */
static int buildHeader(OBJECT_TYPE type, char* file, char* ret)
{
	int size;
	FILE* fp = fopen(file, "rb");

	if(fp == NULL) return -1;

	// seeking until the end of the file to find size
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);

	fclose(fp);
	// return this john in the form "blob <size>\0"
	sprintf(ret, "blob %i", size);
	return size;
}

static uint8_t* buildBuffer(OBJECT_TYPE type, char* file, size_t* outLen)
{
	// create our header 
	char header[50];
	int fileSize = buildHeader(type, file, header);
	int headerSize = strlen(header) + 1; //+1 for the \0
	if(fileSize == -1) return NULL;

	// init our dataBuffer
	uint8_t* dataBuffer = (uint8_t*)malloc(fileSize);
	if(dataBuffer == NULL) return NULL;

	// read file contents and store into dataBuffer
	FILE* fp = fopen(file, "rb");
	if(fp == NULL) return NULL;
	
	fread(dataBuffer, sizeof(dataBuffer[0]), fileSize, fp);
	fclose(fp);

	// copy into final complete buffer
	uint8_t* outBuffer = (uint8_t*)malloc(fileSize + headerSize);

	memcpy(outBuffer, header, headerSize);
	memcpy(outBuffer + headerSize, dataBuffer, fileSize);

	// output the outputs
	*outLen = headerSize + fileSize;
	return outBuffer;
}

uint8_t* hashBlob(char* file)
{
	// ERROR CHECKS 
	// not a tit repo:
	DIR* dir = opendir(".tit");
	if(!dir)
	{
		if(ENOENT == errno) 
		{
			printf("Current directory is not a tit repository\n");
		}
		else
		{
			perror("opendir");
		}
		return NULL;
	}
	closedir(dir);

	// not a valid file
	FILE* fp = fopen(file, "rb");
	if(fp == NULL) 
	{
		printf("%s is not a valid file\n", file);
		return NULL;
	}
	fclose(fp);

	size_t buffSize;

	uint8_t* buffer = buildBuffer(BLOB, file, &buffSize);
	uint8_t* hash = (uint8_t*)calloc(SHA_DIGEST_LENGTH, sizeof(uint8_t));
	SHA1(buffer, buffSize, hash);

	free(buffer);
	return hash;
}

int compressBlob(char* fileIn, char* fileOut)
{
	FILE* inFile = fopen(fileIn, "rb");
	FILE* outFile = fopen(fileOut, "wb");

	// if either are NULL
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

// HASH IT
// MAKE THE FILE (with hash as the filename)
// compress and store in the file.


void test_hash(OBJECT_TYPE type, char* file)
{
	size_t buffSize = 0;
	uint8_t* buffer = buildBuffer(type, file, &buffSize);
	printf("0x");
	for(int i = 0; i < buffSize; i++)
	{
		printf("%x", buffer[i]);
	}

	uint8_t* hash = hashBlob(file);

	printf("\n\nHashed: ");
	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		printf("%x", hash[i]);
	}

	compressBlob("src/tit.c", "src/compressed.z");

	free(buffer);
	free(hash);
}
