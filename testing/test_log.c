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
	commit("commit 1");
	putc('\n', stdout);

	addFile("compile_flags.txt", &entries, &count);
	commit("commit 2");
	putc('\n', stdout);

	addFile("GNUmakefile", &entries, &count);
	commit("commit 3");
	putc('\n', stdout);

	logCommits();
	freeEntriesArr(&entries, count);

	return 0;
}
