#ifndef KODI_SWITCH_DLFCN_H
#define KODI_SWITCH_DLFCN_H

#ifdef __cplusplus
extern "C" {
#endif

#define RTLD_LAZY 0x00001
#define RTLD_NOW 0x00002
#define RTLD_GLOBAL 0x00100
#define RTLD_LOCAL 0x00000
#define RTLD_DEFAULT ((void*)0)

static inline void* dlopen(const char* filename, int flag)
{
  (void)filename;
  (void)flag;
  return 0;
}

static inline int dlclose(void* handle)
{
  (void)handle;
  return -1;
}

static inline void* dlsym(void* handle, const char* symbol)
{
  (void)handle;
  (void)symbol;
  return 0;
}

static inline const char* dlerror(void)
{
  return "dlfcn unavailable on Switch";
}

#ifdef __cplusplus
}
#endif

#endif
