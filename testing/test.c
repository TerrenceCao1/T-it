#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>
#include "init.h"
#include "add.h"

int main(void)
{
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));
	init(cwd);

	initIndex();

	struct indexEntry* entries = NULL;
	size_t count = 0;
	readIndex(&entries, &count);

	addFile("README.md", &entries, &count);
	addFile("src/add.c", &entries, &count);
	addFile("inc/add.h", &entries, &count);
	readIndex(&entries, &count);

	freeEntriesArr(&entries, count);

	return 0;
}
