#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>
#include "tit.h"

int main(void)
{
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));

	init(cwd);
	return 0;
}
