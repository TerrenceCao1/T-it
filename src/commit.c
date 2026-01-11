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
uint8_t* findParentCommit(void)
{
	// if HEAD doesn't exist, this is bad...
	FILE* fp = fopen(".tit/HEAD", "rb");
	if(!fp)
	{
		fclose(fp);
		return NULL;
	}

	// Get the file path written in HEAD
	char filePathBuffer[100];
	fseek(fp, 0, SEEK_END);
	int HEADLen = ftell(fp);
	fread(filePathBuffer, HEADLen, 1, fp);
	fclose(fp);

	// go to the path and read the SHA1
	fp = fopen(filePathBuffer, "rb");
	if(!fp)
	{
		fclose(fp);
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


