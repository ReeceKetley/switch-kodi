set(SOURCES MusicInfoTag.cpp
            MusicInfoTagLoaderDatabase.cpp
            MusicInfoTagLoaderFactory.cpp
            MusicInfoTagLoaderFFmpeg.cpp
            MusicInfoTagLoaderShn.cpp
            ReplayGain.cpp)

set(HEADERS ImusicInfoTagLoader.h
            MusicInfoTag.h
            MusicInfoTagLoaderDatabase.h
            MusicInfoTagLoaderFactory.h
            MusicInfoTagLoaderFFmpeg.h
            MusicInfoTagLoaderShn.h
            ReplayGain.h)

if(ENABLE_OPTICAL)
  list(APPEND SOURCES MusicInfoTagLoaderCDDA.cpp)
  list(APPEND HEADERS MusicInfoTagLoaderCDDA.h)
endif()

if(TAGLIB_FOUND)
  list(APPEND SOURCES TagLibVFSStream.cpp
                      TagLoaderTagLib.cpp)
  list(APPEND HEADERS TagLibVFSStream.h
                      TagLoaderTagLib.h)
endif()

core_add_library(music_tags)
if(TAGLIB_FOUND)
  target_compile_definitions(music_tags PRIVATE HAVE_TAGLIB=1)
endif()
