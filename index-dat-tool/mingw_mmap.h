/** This code is adapted from:
  * https://kerneltrap.org/mailarchive/git/2008/11/21/4186494
  * Original code by Vasyl Vavrychuk.
  *
  * This file is part of Rockstars.
  * Coded in Hungarian Notation so it looks stupid. :D
  *
  * If you read nothing from this file, know NOT to call the function names
  * below but the original mmap() and munmap() functions.
  */

#ifndef _MINGW_MMAP_H
#define _MINGW_MMAP_H

#include <io.h>
#include <windows.h>

#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_SHARED 2
#define MAP_PRIVATE 3

void *mingw_mmap(void *pStart, size_t sLength, int nProt, int nFlags, int nFd, off_t oOffset);
#define mmap mingw_mmap

int mingw_munmap(void *pStart, size_t sLength);
#define munmap mingw_munmap

#endif /* _MINGW_MMAP_H */
