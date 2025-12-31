#include <openssl/sha.h>
#include <sha2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>
#include "tit.h"

int main(int argc, char** argv)
{
	if(argc == 1) // someone only calls "tit" with no cmd
	{
		printf("tit is a version control platform potentially inspired by fairly unpopular, unknown software git.\n\nUsage: try 'tit init'");
		return 0;
	}

	// someone tries to do tit commands without initing
	if((opendir(".tit") == NULL) && (strcmp(argv[1], "init") != 0))
	{
		printf("You need to do 'tit init' in order to use any tit functions\n");
		return 0;
	}

	// INIT
	if(strcmp(argv[1], "init") == 0)
	{
		char cwd[PATH_MAX];
		getcwd(cwd, sizeof(cwd));
		init(cwd);
		return 0;
	}

	// HASH_BLOB
	if(strcmp(argv[1], "hash-object") == 0)
	{
		if(argc < 3) // they didn't include a file
		{
			printf("Please input a file to hash!\n");
			return -1;
		}
		
		uint8_t* hash = NULL;
		// flag and file handeling:
		_Bool write = FALSE;
		_Bool fileFoundFlag = FALSE;
		int fileIndex;

		for(int i = 2; i < argc; i++)
		{
			if(strcmp(argv[i], "-w") == 0)
			{
				write = TRUE;
			}
			if(fopen(argv[i], "rb") != NULL)
			{
				fileFoundFlag = TRUE;
				fileIndex = i;
			}
			if((i == argc - 1) && (fileFoundFlag == FALSE)) 
			{
				printf("Please input a file to hash!\n");
				return -1;
			}
		}
		if(fileFoundFlag)
		{
			hash = hashBlob(argv[fileIndex], write);
			if(hash == NULL) return -1;

			// print out the hash!
			for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
			{
				printf("%02x", hash[i]);
			}
		}
		else
		{
			printf("We can't hash-object without a valid file bub.\n");
		}

		return 0;
	}

	// CAT-FILE for blobs
	if(strcmp(argv[1], "cat-file") == 0)
	{
		// for blobs: 
		// -t means type
		// -s means size
		// blob means print contents
		_Bool typeFlag = FALSE;
		_Bool sizeFlag = FALSE;
		_Bool hashFoundFlag = FALSE;
		_Bool blobFlag = FALSE;
		int hashIndex = -1;

		for(int i = 2; i < argc; i++)
		{
			if(strcmp(argv[i], "-t") == 0)
			{
				typeFlag = TRUE;
			}
			else if(strcmp(argv[i], "-s") == 0)
			{
				sizeFlag = TRUE;
			}
			else if(strcmp(argv[i], "blob") == 0)
			{
				blobFlag = TRUE;
			}
			else // anything will input as hash - handle errors in the catFile Function.
			{
				hashFoundFlag = TRUE;
				hashIndex = i;
			}
		}
		
		if(!hashFoundFlag)
		{
			printf("Input a hash for a file please.\n");
			return -1;
		}
		catFile(argv[hashIndex], typeFlag, sizeFlag, blobFlag);
		return 0;
	}

	else
	{
		printf("INVALID COMMAND or INPUTS!\n");
		return -1;
	}
}
