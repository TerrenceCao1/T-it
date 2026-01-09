#ifndef WRITE_TREE_H
#define WRITE_TREE_H

#include <stdint.h>
#include <stdio.h>

int writeTree(_Bool write);

uint8_t* hashTree(size_t treeSize);

int compressFile(char* fileIn, char* fileOut);
#endif
