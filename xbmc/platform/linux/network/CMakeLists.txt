set(SOURCES NetworkLinux.cpp)

set(HEADERS NetworkLinux.h)

if(CORE_PLATFORM_NAME_LC STREQUAL switch)
  return()
endif()

if(AVAHI_FOUND)
  list(APPEND SOURCES ZeroconfAvahi.cpp
                      ZeroconfBrowserAvahi.cpp)
  list(APPEND HEADERS ZeroconfAvahi.h
                      ZeroconfBrowserAvahi.h)
endif()

core_add_library(platform_linux_network)
