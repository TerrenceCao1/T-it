#include "log.h"
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
#include "cat-file.h"
#include "hash-object.h"
#include "write-tree.h"
#include "commit.h"

/*
* ./tit log looks at the current commit (from HEAD)
* then prints out a little blurb:
* commit <hash>
* Author: blahblahblah <email>
* Date: <Day of Week> <Date> <Time>
*
* <message>
*
* then loops and loops going to the parent commit and then the next and the next
*/

// static parse commit hash (obtain the hash, author, time, and parent hash)
static COMMIT_OBJ* parseCommitFromHash(char* commitHash)
{
	// obtain commit file dir...
	char* directory = obtainObjectFileDir((char*)commitHash);
	
	decompressBlob(directory);
	free(directory);

	FILE* fp = fopen(".tit/temp/out", "rb");
	if(!fp)
	{
		printf("commitHash error. Please Check Spelling.\n");
		return NULL;
	}

	// initiating commit
	COMMIT_OBJ* commit = malloc(sizeof(COMMIT_OBJ));
	commit->hash = commitHash;
	commit->author = NULL;
	commit->message = NULL;
	commit->parent = NULL;

	// trash chars until we get to parent or author
	while(fgetc(fp) != '\n'); // getting to tree
	while(fgetc(fp) != '\n'); // getting to parent/author
	
	// read the word to see which line.
	char temp[7];
	fread(temp, sizeof(char), 6, fp);
	temp[6] = '\0';

	// read the parent hash
	if(strcmp(temp, "parent") == 0)
	{
		fgetc(fp); // burn the space character
		commit->parent = malloc(SHA_DIGEST_LENGTH * 2 + 1);
		fread(commit->parent, sizeof(char), SHA_DIGEST_LENGTH * 2, fp);

		commit->parent[SHA_DIGEST_LENGTH * 2] = '\0';
	}

	// read the author line:
	fgetc(fp); // burn \n
	
	char author[128];
	char email[128];
	long timestamp;
	char timezone[8];

	char line[1024];
	fgets(line, sizeof(line), fp);
	if(sscanf(line, "author %127[^<] <%127[^>]> %ld %7s", author, email, &timestamp, timezone) == 4){
		// for trailing space right after author.
		author[strcspn(author, " ")] = '\0';

		commit->author = strdup(author);
		commit->email = strdup(email);
		commit->time = timestamp;
	}

	return commit;
}

/*
* LOG FUNCTION:
* read the Head commit
* parse it 
* do 
* until we get the parent commit 
*/

int logCommits(void)
{
	char* headCommit = readHead();
	COMMIT_OBJ* currentCommit = parseCommitFromHash(headCommit);

	free(headCommit);
	if(currentCommit)
	{
		free(currentCommit);
	}
	return 0;
}
