#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>
#include <dirent.h>
#include "tit.h"

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
