#ifndef LOG_H
#define LOG_H
#include <stdint.h>
#include <time.h>

typedef struct COMMIT_OBJ
{
	char* hash;
	char* author;
	char* message;
	char* email;
	size_t time;
	char* parent;
} COMMIT_OBJ;

int logCommits(void);

#endif
