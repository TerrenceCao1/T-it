#ifndef WRITE_TREE_H
#define WRITE_TREE_H

#include <stdint.h>
#include <stdio.h>

int writeTree(void);

uint8_t* hashTree(size_t treeSize);

int compressFile(char* fileIn, char* fileOut);
#endif
