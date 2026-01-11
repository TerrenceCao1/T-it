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

// STEPS:
// write the tree and store hash in commit obj
// read the HEAD to find last commit
//		if file exists, that is the parent SHA
// build commit object (author, email, time, message)
// hash and store commit
// update HEAD

// finds the parent commit from HEAD
static uint8_t* findParentCommit(void)
{
	// if HEAD doesn't exist, this is bad...
	FILE* fp = fopen(".tit/HEAD", "rb");
	if(!fp)
	{
		return NULL;
	}

	// Get the file path written in HEAD
	char filePathBuffer[100];
	fseek(fp, 0, SEEK_END);
	int HEADLen = ftell(fp);

	rewind(fp);
	fread(filePathBuffer, HEADLen, 1, fp);
	fclose(fp);

	// go to the path and read the SHA1
	fp = fopen(filePathBuffer, "rb");
	if(!fp)
	{
		return NULL; // if the file pointed to in HEAD doesn't exist, then it's generally ok... we will create the file later in the commit function.
	}

	// ERROR CHECK if the data in filePathBuffer isn't 20 things long, it isn't a valid SHA1.
	fseek(fp, 0, SEEK_END);
	int shaLen = ftell(fp);
	if(shaLen != SHA_DIGEST_LENGTH)
	{
		fclose(fp);
		return NULL;
	}

	// read the sha1 hash
	uint8_t* sha1Hash = malloc(SHA_DIGEST_LENGTH);
	fread(sha1Hash, SHA_DIGEST_LENGTH, 1, fp);
	fclose(fp);

	return sha1Hash;
}

static uint8_t* hashCommit(size_t commitSize)
{
	uint8_t* hash = (uint8_t*)calloc(SHA_DIGEST_LENGTH, sizeof(uint8_t));
	uint8_t buffer[commitSize];

	FILE* fp = fopen(".tit/temp/commitWriter", "rb");
	if(!fp) return NULL;

	// populate buffer
	fread(buffer, sizeof(buffer[0]), commitSize, fp);
	fclose(fp);

	SHA1(buffer, commitSize, hash);

	return hash;
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
	FILE* fp = fopen(".tit/temp/commitWriter", "ab");
	if(!fp)
	{
		return -1;
	}

	// tree and tree hash in format: "tree <tree_sha>"
	uint8_t* treeHash = writeTree(TRUE);
	if(!treeHash)
	{
		printf("Error with treeHash and writeTree function...");
		return -1;
	}

	// actually write that john "tree <tree_sha>"
	fprintf(fp, "tree ");
	fwrite(treeHash, SHA_DIGEST_LENGTH, 1, fp);
	free(treeHash);

	// parent and parent hash
	uint8_t* parentHash = findParentCommit();
	if(parentHash != NULL) // THIS IS NOT AN ERROR, it's the initial commit
	{
		fprintf(fp, "\nparent ");
		fwrite(parentHash, SHA_DIGEST_LENGTH, 1, fp);
		free(parentHash);
	}

	// AUTHOR AND COMMITTER... imma hard code this one
	fprintf(fp, "\nauthor lil_bro <littlebrother@thisisascam.com> 1700000000 -0800");
	fprintf(fp, "\ncommitter lil_bro <littlebrother@thisisascam.com> 1700000000 -0800");

	// Message (error checking the message will happen in CLI)
	fprintf(fp, "\n\n%s", message);

	fclose(fp);

	// HASH AND STORE THE COMMIT!
	fp = fopen(".tit/temp/commitWriter", "rb");
	fseek(fp, 0, SEEK_END);
	size_t commitSize = ftell(fp);
	fclose(fp);

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
		if(compressFile(".tit/temp/commitWriter", finalDir) != Z_OK)
		{
			return -1;
		}
	}
	free(hash);

	// UPDATE HEAD! 
	

	return 0;
}
