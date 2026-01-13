#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>
#include <dirent.h>
#include <openssl/sha.h>
#include <assert.h>
#include <arpa/inet.h>
#include <zlib.h>
#include "cat-file.h"

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

char* obtainObjectFileDir(char* hash)
{
	if(strlen(hash) != SHA_DIGEST_LENGTH * 2)
	{
		printf("Invalid Hash. Hash must be 40 chars long\n");
		return NULL;
	}

	char* directory = malloc(20 + SHA_DIGEST_LENGTH * 2 + 1); // 4 for .tit, 1 for /, 7 for objects, 2 for /'s, SHA_DIGEST_LENGTH * 2 for hash, 1 for NULL
	
	sprintf(directory, ".tit/objects/");
	char folder[4];
	sprintf(folder, "%c%c/", hash[0], hash[1]);
	strcat(directory, folder);

	memcpy(directory + strlen(directory), hash + 2, 2 * SHA256_192_DIGEST_LENGTH - 2);
	directory[strlen(directory)] = '\0';
	
	return directory;
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
	char* hashedFileName = obtainObjectFileDir(hash);

	// decompress the contents of the file (gets saved to binary file ".tit/temp/out")
	if(decompressBlob(hashedFileName) != Z_OK) return -1;

	free(hashedFileName);

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
