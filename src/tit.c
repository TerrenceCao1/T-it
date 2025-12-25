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

// creates a object based on the file given
static int writeObject(char* file)
{
	uint8_t* hash = hashBlob(file, NULL);
	char dir[3]; // 2 for the hex and one for the null term.
	sprintf(dir, "%02x", hash[0]);
	
	char finalDir[100] = ".tit/objects/";
	strcat(finalDir, dir);
	strcat(finalDir, "/");

	if(mkdir(finalDir, FILE_PERMS) == 0)
	{
		char fileName[SHA_DIGEST_LENGTH * 2] = "";
		char temp[3];
		for(int i = 2; i < SHA_DIGEST_LENGTH; i++)
		{
			sprintf(temp, "%02x", hash[i]);
			strcat(fileName, temp);
		}

		strcat(finalDir, fileName);
		FILE* fp = fopen(finalDir, "wb");
		if(fp == NULL)
		{
			perror(fileName);
			return -1;
		}
		printf("directory made: %s\n", finalDir);

		compressBlob(file, finalDir);
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
		writeObject(file);
	}

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

int decompressBlob(char* fileIn)
{
	FILE* inFile = fopen(fileIn, "rb");

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
			if(fwrite(bufferOut, 1, have, /*TODO: modify so that it just prints out and deletes this output file.*/))

		}
	}



}

int catFile(char* hash)
{
	// find the file corresponding to the hash in the objects folder
	// decompress contents
	// output it to stdout
	
	// ERROR CHECKS:
	// hash length must be 40.
	if(strlen(hash) != SHA_DIGEST_LENGTH * 2)
	{
		printf("Invalid Hash. Hash must be 40 chars long\n");
		return -1;
	}
	char file[80]; // ".tit/objects/xx/<38 hex digits>"
	sprintf(file, ".tit/objects/");

	// add the folder to the file location template
	char folder[4];
	sprintf(folder, "%c%c/", hash[0], hash[1]);
	strcat(file, folder);

	memcpy(file + strlen(file), hash + 2, 2 * SHA_DIGEST_LENGTH - 2);

	FILE* fp = fopen(file, "rb");
	if(fp == NULL)
	{
		perror(file);
		return -1;
	}


}


/*
void test_hash(OBJECT_TYPE type, char* file)
{
	size_t buffSize = 0;
	uint8_t* buffer = buildBuffer(type, file, &buffSize);
	printf("0x");
	for(int i = 0; i < buffSize; i++)
	{
		printf("%x", buffer[i]);
	}

	uint8_t* hash = hashBlob(file, 0);

	printf("\n\nHashed: ");
	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		printf("%x", hash[i]);
	}

	compressBlob("src/tit.c", "src/compressed.z");

	free(buffer);
	free(hash);
}
*/
