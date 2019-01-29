/** This code is adapted from:
  * https://kerneltrap.org/mailarchive/git/2008/11/21/4186494
  * Original code by Vasyl Vavrychuk.
  *
  * This file is part of Rockstars.
  * Coded in Hungarian Notation so it looks stupid. :D
  *
  * If you haven't seen the header file, call mmap() and munmap() not the
  * function names below!
  */

#include <stdio.h>
#include "mingw_mmap.h"

extern int getpagesize();

/**
  * Use CreateFileMapping and MapViewOfFile to simulate POSIX mmap().
  * Why Microsoft won't just implement these is beyond everyone's comprehension.
  * @return pointer or NULL
  */
void *mingw_mmap(void *pStart, size_t sLength, int nProt, int nFlags, int nFd, off_t oOffset) {
  (void)nProt;
  HANDLE hHandle;

  if (pStart != NULL || !(nFlags & MAP_PRIVATE)) {
	printf("Invalid usage of mingw_mmap");
    return NULL;
  }

  if (oOffset % getpagesize() != 0) {
    printf("Offset does not match the memory allocation granularity");
    return NULL;
  }

  hHandle = CreateFileMapping((HANDLE)_get_osfhandle(nFd), NULL, PAGE_WRITECOPY, 0, 0, NULL);
  if (hHandle != NULL) {
    pStart = MapViewOfFile(hHandle, FILE_MAP_COPY, 0, oOffset, sLength);
  }

  return pStart;
}

/**
  * Use UnmapViewOfFile to undo mmap() above.
  * @param pStart
  * @param length - Not used, kept for compatibility.
  * @return boolean; no checks are performed.
  */
int mingw_munmap(void *pStart, size_t sLength) {
  (void)sLength;

  if (UnmapViewOfFile(pStart) != 0)
    return FALSE;

  return TRUE;
}
