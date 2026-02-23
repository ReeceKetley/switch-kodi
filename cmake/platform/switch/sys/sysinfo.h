#ifndef KODI_SWITCH_SYS_SYSINFO_H
#define KODI_SWITCH_SYS_SYSINFO_H

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sysinfo
{
  long uptime;
  unsigned long loads[3];
  unsigned long totalram;
  unsigned long freeram;
  unsigned long sharedram;
  unsigned long bufferram;
  unsigned long totalswap;
  unsigned long freeswap;
  unsigned short procs;
  unsigned long totalhigh;
  unsigned long freehigh;
  unsigned int mem_unit;
};

static inline int sysinfo(struct sysinfo* info)
{
  if (info)
  {
    info->uptime = 0;
    info->loads[0] = 0;
    info->loads[1] = 0;
    info->loads[2] = 0;
    info->totalram = 0;
    info->freeram = 0;
    info->sharedram = 0;
    info->bufferram = 0;
    info->totalswap = 0;
    info->freeswap = 0;
    info->procs = 1;
    info->totalhigh = 0;
    info->freehigh = 0;
    info->mem_unit = 1;
  }
  errno = ENOSYS;
  return -1;
}

#ifdef __cplusplus
}
#endif

#endif
