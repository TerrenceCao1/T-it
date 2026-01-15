#include <stdint.h>
#include <time.h>
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
#include "cat-file.h"
#include "hash-object.h"
#include "write-tree.h"

// CREATING THE COMMIT OBJECT HAS THIS FORMAT:
/*
* tree <tree_sha>
* parent <parent_commit_sha>
* author Name <email> timestamp
* committer Name <email> timestamp
*
* <commit message>
*/

// COMMIT HASH:
// "commit <size>\0" + <commit_content>

// finds the parent commit from HEAD
char* readHead(void)
{
	// if HEAD doesn't exist, this is bad...
	FILE* fp = fopen(".tit/HEAD", "rb");
	if(!fp)
	{
		return NULL;
	}

	// Get the file path written in HEAD
	fseek(fp, 0, SEEK_END);
	int HEADLen = ftell(fp);
	char filePathBuffer[HEADLen - 1]; // cause there's a new line...

	rewind(fp);
	fread(filePathBuffer, HEADLen, 1, fp);
	fclose(fp);

	filePathBuffer[HEADLen - 1] = '\0';
	char finalDir[100] = ".tit/";
	strcat(finalDir, filePathBuffer);

	// go to the path and read the SHA1
	fp = fopen(finalDir, "rb");
	if(!fp)
	{
		return NULL; // if the file pointed to in HEAD doesn't exist, then it's generally ok... we will create the file later in the commit function.
	}

	// ERROR CHECK if the data in filePathBuffer isn't 20 things long, it isn't a valid SHA1.
	fseek(fp, 0, SEEK_END);
	int shaLen = ftell(fp);
	if(shaLen != SHA_DIGEST_LENGTH * 2 + 1)
	{
		fclose(fp);
		return NULL;
	}
	rewind(fp);

	// read the sha1 hash
	char* sha1Hash = malloc(SHA_DIGEST_LENGTH * 2 + 1);
	fread(sha1Hash, SHA_DIGEST_LENGTH * 2, 1, fp);
	sha1Hash[SHA_DIGEST_LENGTH * 2] = '\0';
	fclose(fp);

	return sha1Hash;
}

static uint8_t* hashCommit(size_t commitSize)
{
	uint8_t* hash = (uint8_t*)calloc(SHA_DIGEST_LENGTH, sizeof(uint8_t));
	uint8_t buffer[commitSize];

	FILE* fp = fopen(".tit/temp/finalCommit", "rb");
	if(!fp) return NULL;

	// populate buffer
	fread(buffer, sizeof(buffer[0]), commitSize, fp);
	fclose(fp);

	SHA1(buffer, commitSize, hash);

	return hash;
}

static void clearFile(char* file)
{
	FILE* fp = fopen(file, "w");
	if(fp)
	{
		fclose(fp);
	}
	return;
}

/*
 * COMMIT FORMAT:
 * tree <tree_sha>
 * parent <parent_hash> ***IF EXISTS
 * author <name, email, timestamp> imma just use a default name like bro or sum
 * committer <name, email, timestamp> usually same as author
 * NEW LINE
 * commit message
*/
int commit(char* message)
{
	// create temp file that we will manipulate to create a commit!
	clearFile(".tit/temp/commitWriter");
	clearFile(".tit/temp/finalCommit");
	FILE* fp = fopen(".tit/temp/commitWriter", "ab");
	if(!fp)
	{
		return -1;
	}

	uint8_t* treeHash = writeTree(TRUE);
	if(!treeHash)
	{
		printf("Error with treeHash and writeTree function...");
		return -1;
	}

	// actually write that john "tree <tree_sha>"
	fprintf(fp, "\ntree ");
	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		fprintf(fp, "%02x", treeHash[i]);
	}
	free(treeHash);

	// parent and parent hash
	char* parentHash = readHead();
	if(parentHash != NULL)
	{
		fprintf(fp, "\nparent ");
		fwrite(parentHash, SHA_DIGEST_LENGTH * 2, 1, fp);
		free(parentHash);
	}

	// AUTHOR AND COMMITTER... imma hard code this one
	fprintf(fp, "\nauthor Tcow <tcow@thisisnotarealemail.com> %u -0800", (unsigned)time(NULL));
	fprintf(fp, "\ncommitter Tcow <tcow@thisisnotarealemail.com> %u -0800", (unsigned)time(NULL));

	// Message (error checking the message will happen in CLI)
	fprintf(fp, "\n\n%s", message);

	fclose(fp);

	// HASH AND STORE THE COMMIT!
	fp = fopen(".tit/temp/commitWriter", "rb");
	fseek(fp, 0, SEEK_END);
	size_t commitSize = ftell(fp);
	fclose(fp);

	// final commit with header!
	FILE* finalCommitFile = fopen(".tit/temp/finalCommit", "ab");
	if(!finalCommitFile)
	{
		return -1;
	}
	fprintf(finalCommitFile, "commit %lu", commitSize);
	fputc('\0', finalCommitFile);

	FILE* sourceFile = fopen(".tit/temp/commitWriter", "rb");
	if(!sourceFile)
	{
		return -1;
	}
	char ch;
	while((ch = fgetc(sourceFile)) != EOF)
	{
		fputc(ch, finalCommitFile);
	}

	fclose(finalCommitFile);
	fclose(sourceFile);

	uint8_t* hash = hashCommit(commitSize);

	// make the directory for the commit to live in
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

		strcat(finalDir, fileName);
		FILE* fp = fopen(finalDir, "wb");
		if(fp == NULL)
		{
			perror(fileName);
			return -1;
		}

		// compress the file and write to the object file
		if(compressFile(".tit/temp/finalCommit", finalDir) != Z_OK)
		{
			return -1;
		}
		fclose(fp);
	}

	// UPDATE HEAD! 
	// if HEAD doesn't exist, this is bad...
	fp = fopen(".tit/HEAD", "rb");
	if(!fp)
	{
		return -1;
	}

	// Get the file path written in HEAD
	fseek(fp, 0, SEEK_END);
	int HEADLen = ftell(fp);
	char filePathBuffer[HEADLen];

	rewind(fp);
	fread(filePathBuffer, HEADLen, 1, fp);
	filePathBuffer[HEADLen - 1] = '\0';
	fclose(fp);

	char fullPath[256] = ".tit/";
	strcat(fullPath, filePathBuffer);

	// go to the path and read the SHA1
	fp = fopen(fullPath, "wb");
	if(!fp)
	{
		return -1;
	}

	// write the hash as hex text, not pure hash binaries
	char hashOut[SHA_DIGEST_LENGTH * 2 + 2];
	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		sprintf(hashOut + (i * 2), "%02x", hash[i]);
	}
	hashOut[40] = '\n';
	hashOut[41] = '\0';

	fwrite(hashOut, 41, 1, fp);
	fclose(fp);

	// for printing stuff out when we commit!
	char shortHash[8];
	memcpy(shortHash, hashOut, 7);
	shortHash[7] = '\0';

	printf("committed [%s] %s", shortHash, message);

	free(hash);
	return 0;
}
