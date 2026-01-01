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
	if(mkdirAtPath(".tit/temp") != 0) return -1;

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

// TIT CAT-FILE and HELPER FUNCTIONS:


int decompressBlob(char* fileIn)
{
	FILE* inFile = fopen(fileIn, "rb");
	if(inFile == NULL)
	{
		printf("Invalid File!\n");
		return -1;
	}
	FILE* outFile = fopen(".tit/temp/out", "wb");

	int ret;
	unsigned have;
	z_stream strm;
	unsigned char bufferIn[CHUNK];
	unsigned char bufferOut[CHUNK];

	// setting inflate parameters
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;

	ret = inflateInit(&strm);
	if(ret != Z_OK)
	{
		return ret;
	}

	do
	{
		strm.avail_in = fread(bufferIn, 1, CHUNK, inFile);
		if(ferror(inFile))
		{
			(void)inflateEnd(&strm);
			return Z_ERRNO;
		}
		if(strm.avail_in == 0)
		{
			break;
		}
		strm.next_in = bufferIn;

		do
		{
			strm.avail_out = CHUNK;
			strm.next_out = bufferOut;

			ret = inflate(&strm, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);
			
			switch(ret)
			{
				case Z_NEED_DICT: // FALL THROUGH!
					ret = Z_DATA_ERROR;
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					return ret;
			}

			have = CHUNK - strm.avail_out;
			if(fwrite(bufferOut, 1, have, outFile) != have || ferror(outFile))
			{
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);
	} while (ret != Z_STREAM_END);

	(void)inflateEnd(&strm);
	fclose(inFile);
	fclose(outFile);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int catFile(char* hash, _Bool type, _Bool size, _Bool blob)
{
	// ERROR CHECKS:
	// hash length must be 40.
	if(strlen(hash) != SHA_DIGEST_LENGTH * 2)
	{
		printf("Invalid Hash. Hash must be 40 chars long\n");
		return -1;
	}
	char hashedFileName[80]; // ".tit/objects/xx/<38 hex digits>"
	sprintf(hashedFileName, ".tit/objects/");

	// add the folder to the hashedFileName location template
	char folder[4];
	sprintf(folder, "%c%c/", hash[0], hash[1]);
	strcat(hashedFileName, folder);

	memcpy(hashedFileName + strlen(hashedFileName), hash + 2, 2 * SHA_DIGEST_LENGTH - 2);
	hashedFileName[strlen(hashedFileName)] = '\0';

	// decompress the contents of the file (gets saved to binary file ".tit/temp/out")
	if(decompressBlob(hashedFileName) != Z_OK) return -1;

	// then we print out all the contents of the decompressed file
	FILE* fp = fopen(".tit/temp/out", "rb");
	if(fp == NULL)
	{
		perror(".tit/temp/out");
		return -1;
	}

	// FOR TYPE:
	if(type)
	{
		int c;
		// print characters until a space
		while((c = getc(fp)) != ' ')
		{
			putchar(c);
		}
		printf("\n");
	}

	// FOR SIZE:
	if(size)
	{
		fp = fopen(".tit/temp/out", "rb");
		if(fp == NULL)
		{
			perror(".tit/temp/out");
			return -1;
		}

		// start printing after the space and before \0
		int c;
		while((c = getc(fp)) != ' ')
		{
			continue; // run the fp STREAM until the char is a space
		}
		while((c = getc(fp)) != '\0')
		{
			putchar(c);
		}
		printf("\n");
	}

	// FOR BLOB:
	if(blob)
	{
		fp = fopen(".tit/temp/out", "rb");
		if(fp == NULL)
		{
			perror(".tit/temp/out");
			return -1;
		}

		unsigned char buffer[1024];
		size_t n;
		int seen_nul = 0;

		fflush(stdout);
		while((n = fread(buffer, 1, sizeof(buffer), fp)) > 0)
		{
			if(!seen_nul)
			{
				size_t i;
				for(i = 0; i < n; i++)
				{
					if(buffer[i] == '\0')
					{
						seen_nul = 1;
						i++;
						break;
					}
				}
				if(seen_nul && i < n)
				{
					fwrite(buffer + i, 1, n - i, stdout);
				}
			}
			else
			{
				fwrite(buffer, 1, n, stdout);
			}
		}

		fflush(stdout);
		fclose(fp);
	}
	return 0;
}

// TIT ADD and Helper Functions:
/* tit add will:
 * write an object file for the file added
 * create an indexEntry - with mode, sha1 hash, path, file_size
 * write the indexEntry to the .tit/index file
 */

// this function initiates an index header (caller must free!), and makes the index file
// There is only ONE index header (that is global and updates whenever an entry is added/rmed)
static indexHeader* writeIndexHeader(void)
{
	// making header defaults
	struct indexHeader* header = malloc(sizeof(indexHeader));
	memcpy(header->signature, "DIRC", 4);
	header->version = htonl(2);
	header->entry_count = htonl(0);

	// creating the index file
	FILE* fp = fopen(".tit/index", "ab");
	if(fp == NULL)
	{
		perror(".tit/index");
		return NULL;
	}

	// write the header to the index file:
	fwrite(header->signature, 1, 4, fp);
	fwrite(&header->version, 4, 1, fp);
	fwrite(&header->entry_count, 4, 1, fp);

	fclose(fp);
	return header;
}

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

indexHeader* initIndex(void)
{
	indexHeader* header = writeIndexHeader();
	writeChecksum();

	return header;
}

// create an indexEntry from a path to an object
indexEntry* createIndexEntry(char* filePath)
{
	// make the object file
	char* hashedObjectPath;
	writeObject(filePath, hashedObjectPath);

	// initialize an indexEntry, with the stuff from the file we have
	indexEntry* entry = malloc(sizeof(struct indexEntry));
	if(entry == NULL) return NULL;

	entry->mode = FILE_PERMS;
	entry->pathLen = strlen(filePath);
	entry->path = filePath;
	entry->file_size = getFileSize(filePath);

	uint8_t* sha1 = hashBlob(filePath, FALSE);
	memcpy(entry->sha1, sha1, SHA_DIGEST_LENGTH);

	free(hashedObjectPath);
	return entry;
}

// add an indexEntry to the end of the .tit/index file
int addEntryToIndex(indexHeader* header, indexEntry* entry)
{
	return 0;
}

// readIndex reads the file at .tit/index and outputs an array of entries in the file, and the count of them.
static int readIndex(struct indexEntry** entries, size_t* count);
