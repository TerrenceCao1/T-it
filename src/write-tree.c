#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>
#include <dirent.h>
#include <openssl/sha.h>
#include <assert.h>
#include <arpa/inet.h>
#include <zlib.h>
#include "add.h"
#include "hash-object.h"
#include "write-tree.h"

// Used for sorting our index
static int cmp_entries(const void *a, const void *b)
{
	const indexEntry *ea = a;
	const indexEntry *eb = b;
	return strcmp(ea->path, eb->path);
}

int writeTree(void)
{
	// get our entries
	struct indexEntry* entries;
	size_t count;
	readIndex(&entries, &count);

	// must be sorted
	qsort(entries, count, sizeof(entries[0]), cmp_entries);

	// write relevant info to a temp file
	FILE* fp = fopen(".tit/temp/treeWriter", "wb");
	if(!fp) return -1;

	// for each entry we write:
	// MODE
	// a space
	// FILENAME
	// null byte
	// SHA1 Hash
	for(int i = 0; i < count; i++)
	{
		fwrite(&((entries)[i].mode), 4, 1, fp);
		fputc(' ', fp);
		fwrite((entries)[i].path, ((entries)[i].pathLen), 1, fp);
		fputc('\0', fp);
		fwrite(&((entries)[i].sha1), 20, 1, fp);
	}
	return 0;
}
