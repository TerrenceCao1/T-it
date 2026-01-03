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

#define INPUT_FILE "./testing/testInput.txt"

int main(void)
{
	// INIT:
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));
	init(cwd);

	DIR* dir = opendir(".tit");
	if(ENOENT == errno)
	{
		printf("didn't make the stuff bub\n");
	}
	else
	{
		printf("Test Passed: init\n");
	}

	printf(".tit contents: \n");
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		printf("%s\n", entry->d_name);
	}

	closedir(dir);

	// hashing
	uint8_t* hash = hashBlob(INPUT_FILE, 1);
	printf("\nMy hash:  ");

	if(hash != NULL)
	{
		for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
		{
			printf("%02x", hash[i]);
		}
	}
	else printf("Test FAILED: hash\n");

	// comparing it to git hash-object
	char git_cmd[100] = "git hash-object ";
	strcat(git_cmd, INPUT_FILE);
	FILE* fp = popen(git_cmd, "r");
	char ch;

	printf("\nGit hash: ");
	if(fp == NULL)
	{
		printf("popen failed\n");
	}
	else
	{
		while((ch = fgetc(fp)) != EOF)
		{
			putchar(ch);
		}
		pclose(fp);
	}

	free(hash);
	return 0;
}
