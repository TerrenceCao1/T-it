/*
 * @file	Header file for the tit init functions!
 *
 * @author	Terrence Cao (TCow-Kachow)
 *
 * @date	Dec 16 2025
 *
 * */

#ifndef INIT_H
#define INIT_H

#include <stdio.h>
#include <stdint.h>

#define FILE_PERMS 0755 // Owner can read/write/exec, Others can read/exed

#define TRUE 1
#define FALSE 0

/*
 * @brief	initializes a tit repo in the specified path
 *			
 *			makes the necessary directories: .tit, .tit/objects, .tit/refs, .tit/refs/heads, .tit/HEAD
 *
 * @param	path - string of the path that we are workin in
 *
 * @return void 
 *
 * */

int init(const char* path);

#endif 
