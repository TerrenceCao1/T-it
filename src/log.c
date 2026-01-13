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
static COMMIT_OBJ* parseCommitFromHash(uint8_t* commitHash)
{
	// obtain commit file dir...
	char* directory = obtainObjectFileDir((char*)commitHash);
	
	FILE* fp = fopen(directory, "rb");
	if(!fp)
	{
		printf("commitHash error. Please Check Spelling.\n");
		return NULL;
	}

	COMMIT_OBJ* commit = malloc(sizeof(COMMIT_OBJ));
	commit->hash = commitHash;

	// first obtain the author
	while(fgetc(fp)!= '\n'); // trash chars until we get to the newLine
	
}

/*
* LOG FUNCTION:
* read the Head commit
* parse it 
* do until we get the parent commit 
*/
