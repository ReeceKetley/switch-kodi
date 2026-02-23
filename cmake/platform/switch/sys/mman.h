#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>

#ifndef PROT_READ
#define PROT_READ 0x1
#endif
#ifndef PROT_WRITE
#define PROT_WRITE 0x2
#endif
#ifndef MAP_SHARED
#define MAP_SHARED 0x01
#endif
#ifndef MAP_PRIVATE
#define MAP_PRIVATE 0x02
#endif
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((void*)-1)
#endif

static inline void* mmap(void* addr, size_t length, int prot, int flags, int fildes, off_t offset)
{
  (void)addr;
  (void)prot;
  (void)flags;
  (void)fildes;
  (void)offset;
  return calloc(1, length);
}

static inline int munmap(void* addr, size_t length)
{
  (void)length;
  free(addr);
  return 0;
}
