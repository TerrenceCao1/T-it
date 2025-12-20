/*
 * @file	Header file for all of the main T-it functions!
 *
 * @author	Terrence Cao (TCow-Kachow)
 *
 * @date	Dec 16 2025
 *
 * */

#ifndef TIT_H
#define TIT_H

#define FILE_PERMS 0755 // Owner can read/write/exec, Others can read/exed

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
