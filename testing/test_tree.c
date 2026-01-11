#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>
#include "init.h"
#include "add.h"
#include "write-tree.h"

int main(void)
{
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));
	init(cwd);

	struct indexEntry* entries = NULL;
	size_t count = 0;
	readIndex(&entries, &count);

	addFile("README.md", &entries, &count);
	addFile("README.md", &entries, &count);
	addFile("src/add.c", &entries, &count);

	uint8_t* treeHash = writeTree(TRUE);

	free(treeHash);
	freeEntriesArr(&entries, count);

	return 0;
}
