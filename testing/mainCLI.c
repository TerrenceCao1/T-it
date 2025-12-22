#include <openssl/sha.h>
#include <sha2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>
#include "tit.h"

int main(int argc, char** argv)
{
	if(argc == 1) // someone only calls "tit" with no cmd
	{
		printf("tit is a version control platform potentially inspired by fairly unpopular, unknown software git.\n");
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
	if(strcmp(argv[1], "hash_blob") == 0)
	{
		if(argc < 3) // they didn't include a file
		{
			printf("Please input a file to hash!\n");
			return -1;
		}
		uint8_t* hash = hashBlob(argv[2]);
		if(hash == NULL) return -1;

		for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
		{
			printf("%02x", hash[i]);
		}
		return 0;
	}
	else
	{
		printf("NOT A COMMAND!\n");
		return -1;
	}
}
