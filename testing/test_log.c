#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>
#include "init.h"
#include "add.h"
#include "write-tree.h"
#include "commit.h"
#include "log.h"

int main(void)
{
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));
	init(cwd);

	struct indexEntry* entries = NULL;
	size_t count = 0;
	readIndex(&entries, &count);

	addFile("README.md", &entries, &count);
	commit("yuh");

	addFile("compile_flags.txt", &entries, &count);
	commit("compile");

	logCommits();
	freeEntriesArr(&entries, count);

	return 0;
}
