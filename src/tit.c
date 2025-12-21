#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>
#include <dirent.h>
#include "tit.h"

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

	//ERROR CHECKS - if path don't exist, or path isn't empty...
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

	initialize_repo(path);
	return 0;
}

// HASH OBJECT STATICS + function
/*
 * build full buffer (type size data)
 * hash it into raw digest
 * conv raw digest into hex
 * derive object path from the hex
 *
 * */
static int buildHeader(OBJECT_TYPE type, char* file, char* ret)
{
	char typeStr[7];
	int size;
	FILE* fp = fopen(file, "r");
	if(fp == NULL)
	{
		perror(file);
		return -1;
	}

	switch(type)
	{
		case BLOB:
			sprintf(typeStr, "blob");
			fseek(fp, 0, SEEK_END);
			size = ftell(fp);
			break;
		case TREE:
			sprintf(typeStr, "tree");
			// TODO: AFTER writing tree code do this
			break;
		case COMMIT:
			sprintf(typeStr, "commit");
			//TODO: After writing commit code do this
			//size = strlen(commit->message)
			break;
	}

	sprintf(ret, "%s %i", typeStr, size);
	return size;
}

static int buildBuffer(OBJECT_TYPE type, char* file)
{
	char header[50];
	int size = buildHeader(type, file, header);
	if(size == -1) //buildHeader ran into error
	{
		return -1;
	}

	uint8_t buffer[sizeof(header) + size];

	FILE* fp = fopen(file, "r");

	int i = 0;
	int c;

	while(header[i] != '\0')
	{
		buffer[i] = header[i];
		i++;
	}
	buffer[sizeof(header)] = '\0';

	i = 0;
	while((c = fgetc(fp)) != EOF)
	{
		buffer[sizeof(header) + i] = c;
		i++;
	}

	for(int i = 0; i < sizeof(buffer); i++)
	{
		printf("%c", buffer[i]);
	}

	printf("\n");

	return 0;
}

void test(OBJECT_TYPE type, char* file, char* ret)
{
	buildHeader(type, file, ret);
	buildBuffer(type, file);
}
