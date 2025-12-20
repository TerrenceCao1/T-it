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
	if(strcmp(argv[1], "init") == 0)
	{
		char cwd[PATH_MAX];
		getcwd(cwd, sizeof(cwd));
		init(cwd);
		return 0;
	}
	else
	{
		printf("NOT A COMMAND!\n");
		return -1;
	}
}
