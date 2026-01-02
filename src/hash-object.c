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
#include <arpa/inet.h>
#include <zlib.h>
#include "hash-object.h"

/*
 * This function builds the header in form: "<type> <size>"
 * */
static char* buildHeader(OBJECT_TYPE type, char* file, size_t* outLen, long* fileSize)
{
	FILE* fp = fopen(file, "rb");

	if(fp == NULL) return NULL;

	// seeking until the end of the file to find size
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);

	fclose(fp);

	int len = snprintf(NULL, 0, "blob %ld", size);

	char* header = malloc(len + 1);
	if(!header) return NULL;

	// return this john in the form "blob <size>\0"
	snprintf(header, len + 1, "blob %ld", size);
	*outLen = len + 1;
	*fileSize = size;
	return header;
}

/*
 * buildBuffer builds a malloced uint8_t buffer with the header, and file contents of a file
 */
static uint8_t* buildBuffer(OBJECT_TYPE type, char* file, size_t* outLen)
{
	// create our header 
	size_t headerLen;
	long fileSize;

	char* header = buildHeader(type, file, &headerLen, &fileSize);
	if(!header) return NULL;

	// init our dataBuffer
	uint8_t* dataBuffer = (uint8_t*)malloc(fileSize);
	if(dataBuffer == NULL) return NULL;

	// read file contents and store into dataBuffer
	FILE* fp = fopen(file, "rb");
	if(fp == NULL) return NULL;
	
	fread(dataBuffer, sizeof(dataBuffer[0]), fileSize, fp);
	fclose(fp);

	// copy into final complete buffer
	uint8_t* outBuffer = (uint8_t*)malloc(fileSize + headerLen);

	memcpy(outBuffer, header, headerLen);
	memcpy(outBuffer + headerLen, dataBuffer, fileSize);

	// output the outputs
	*outLen = headerLen + fileSize;
	return outBuffer;
}

static size_t getFileSize(char* file)
{
	FILE* fp = fopen(file, "rb");
	fseek(fp, 0L, SEEK_END);
	return ftell(fp);
}

// creates a object based on the file given
static int writeObject(char* file, char* finalDirOut)
{
	// construct file path for the object using the hash
	uint8_t* hash = hashBlob(file, NULL);
	char dir[3]; // 2 for the hex and one for the null term.
	sprintf(dir, "%02x", hash[0]);

	char finalDir[100] = ".tit/objects/";
	strcat(finalDir, dir);
	strcat(finalDir, "/");

	if(mkdir(finalDir, FILE_PERMS) == 0)
	{
		// make file name
		char fileName[SHA_DIGEST_LENGTH * 2] = "";
		char temp[3];
		for(int i = 1; i < SHA_DIGEST_LENGTH; i++)
		{
			sprintf(temp, "%02x", hash[i]);
			strcat(fileName, temp);
		}

		// create and edit the finalDir
		strcat(finalDir, fileName);
		FILE* fp = fopen(finalDir, "wb");
		if(fp == NULL)
		{
			perror(fileName);
			return -1;
		}
		// printf("directory made: %s\n", finalDir);

		size_t bufferLen = -1;
		uint8_t* dataBuffer = buildBuffer(BLOB, file, &bufferLen);
		compressBlobBuffer(dataBuffer, bufferLen, finalDir);
	}

	// if we need the outputed file path we have the option
	if(finalDirOut)
	{
		finalDirOut = malloc(strlen(finalDir) + 1);
		strcpy(finalDirOut, finalDir);
	}
	free(hash);
	return 0;
}


uint8_t* hashBlob(char* file, _Bool write)
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

	if(write)
	{
		writeObject(file, NULL);
	}

	free(buffer);
	return hash;
}

int compressBlobBuffer(uint8_t* dataBuffer, size_t dataLen, char* fileOut)
{
	// write the dataBuffer (which will be created by caller using buildBuffer) to a file that we can then compress.
	FILE* dataHelper = fopen(".tit/temp/compressHelper", "wb");
	fwrite(dataBuffer, sizeof(uint8_t), dataLen, dataHelper);
	fclose(dataHelper);

	FILE* inFile = fopen(".tit/temp/compressHelper", "rb");
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
