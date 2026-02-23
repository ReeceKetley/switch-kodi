#.rst:
# FindPCRE
# --------
# Finds the PCRECPP library
#
# This will define the following variables::
#
# PCRE_FOUND - system has libpcrecpp
# PCRE_INCLUDE_DIRS - the libpcrecpp include directory
# PCRE_LIBRARIES - the libpcrecpp libraries
# PCRE_DEFINITIONS - the libpcrecpp definitions
#
# and the following imported targets::
#
#   PCRE::PCRECPP - The PCRECPP library
#   PCRE::PCRE    - The PCRE library

if(ENABLE_INTERNAL_PCRE)
  include(ExternalProject)

  file(STRINGS ${CMAKE_SOURCE_DIR}/tools/depends/target/pcre/Makefile VER REGEX "^[ ]*VERSION[ ]*=.+$")
  string(REGEX REPLACE "^[ ]*VERSION[ ]*=[ ]*" "" PCRE_VERSION "${VER}")

  if(PCRE_URL)
    get_filename_component(PCRE_URL "${PCRE_URL}" ABSOLUTE)
  else()
    set(PCRE_URL http://mirrors.kodi.tv/build-deps/sources/pcre-${PCRE_VERSION}.tar.gz)
  endif()
  if(VERBOSE)
    message(STATUS "PCRE_URL: ${PCRE_URL}")
  endif()

  set(PCRE_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/pcre/include)
  set(PCRE_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/pcre/lib/libpcre.a)
  set(PCRECPP_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/pcre/lib/libpcrecpp.a)
  set(PCRE_HOST_TRIPLET ${ARCH})
  set(PCRE_LD ${CMAKE_LINKER})
  set(PCRE_JIT_OPTION --enable-jit)
  if(CORE_PLATFORM_NAME STREQUAL switch)
    set(PCRE_HOST_TRIPLET aarch64-none-elf)
    set(PCRE_LD /opt/devkitpro/devkitA64/aarch64-none-elf/bin/ld)
    set(PCRE_JIT_OPTION --disable-jit)
  endif()

  ExternalProject_Add(pcre
                      URL ${PCRE_URL}
                      DOWNLOAD_NAME pcre-${PCRE_VERSION}.tar.gz
                      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
                      PREFIX ${CORE_BUILD_DIR}/pcre
                      BUILD_IN_SOURCE 1
                      CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env
                                        AR=${CMAKE_AR}
                                        RANLIB=${CMAKE_RANLIB}
                                        CC=${CMAKE_C_COMPILER}
                                        CXX=${CMAKE_CXX_COMPILER}
                                        LD=${PCRE_LD}
                                        CXXLD=${PCRE_LD}
                                        <SOURCE_DIR>/configure
                                        --host=${PCRE_HOST_TRIPLET}
                                        --prefix=<INSTALL_DIR>
                                        --disable-shared
                                        --disable-stack-for-recursion
                                        --enable-pcre8
                                        --disable-pcre16
                                        --disable-pcre32
                                        ${PCRE_JIT_OPTION}
                                        --enable-utf
                                        --enable-unicode-properties
                                        --enable-newline-is-anycrlf
                      BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} -j1 libpcre.la libpcrecpp.la libpcreposix.la
                      INSTALL_COMMAND ${CMAKE_MAKE_PROGRAM} install-libLTLIBRARIES
                                                      install-includeHEADERS
                                                      install-nodist_includeHEADERS
                                                      install-pkgconfigDATA
                      BUILD_BYPRODUCTS ${PCRE_LIBRARY}
                                       ${PCRECPP_LIBRARY})
  set_target_properties(pcre PROPERTIES FOLDER "External Projects")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PCRE
                                    REQUIRED_VARS PCRECPP_LIBRARY PCRE_LIBRARY PCRE_INCLUDE_DIR
                                    VERSION_VAR PCRE_VERSION)

  set(PCRE_LIBRARIES ${PCRECPP_LIBRARY} ${PCRE_LIBRARY})
  set(PCRE_INCLUDE_DIRS ${PCRE_INCLUDE_DIR})
  if(NOT TARGET PCRE::PCRE)
    add_library(PCRE::PCRE UNKNOWN IMPORTED)
    set_target_properties(PCRE::PCRE PROPERTIES
                                     IMPORTED_LOCATION "${PCRE_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${PCRE_INCLUDE_DIR}")
  endif()
  if(NOT TARGET PCRE::PCRECPP)
    add_library(PCRE::PCRECPP UNKNOWN IMPORTED)
    set_target_properties(PCRE::PCRECPP PROPERTIES
                                        IMPORTED_LOCATION "${PCRECPP_LIBRARY}"
                                        INTERFACE_LINK_LIBRARIES PCRE::PCRE)
  endif()
else()
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_PCRE libpcrecpp QUIET)
  endif()

  find_path(PCRE_INCLUDE_DIR pcrecpp.h
                             PATHS ${PC_PCRE_INCLUDEDIR})
  find_library(PCRECPP_LIBRARY_RELEASE NAMES pcrecpp
                                       PATHS ${PC_PCRE_LIBDIR})
  find_library(PCRE_LIBRARY_RELEASE NAMES pcre
                                    PATHS ${PC_PCRE_LIBDIR})
  find_library(PCRECPP_LIBRARY_DEBUG NAMES pcrecppd
                                     PATHS ${PC_PCRE_LIBDIR})
  find_library(PCRE_LIBRARY_DEBUG NAMES pcred
                                     PATHS ${PC_PCRE_LIBDIR})
  set(PCRE_VERSION ${PC_PCRE_VERSION})

  include(SelectLibraryConfigurations)
  select_library_configurations(PCRECPP)
  select_library_configurations(PCRE)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PCRE
                                    REQUIRED_VARS PCRECPP_LIBRARY PCRE_LIBRARY PCRE_INCLUDE_DIR
                                    VERSION_VAR PCRE_VERSION)

  if(PCRE_FOUND)
    set(PCRE_LIBRARIES ${PCRECPP_LIBRARY} ${PCRE_LIBRARY})
    set(PCRE_INCLUDE_DIRS ${PCRE_INCLUDE_DIR})
    if(WIN32)
      set(PCRE_DEFINITIONS -DPCRE_STATIC=1)
    endif()

    if(NOT TARGET PCRE::PCRE)
      add_library(PCRE::PCRE UNKNOWN IMPORTED)
      if(PCRE_LIBRARY_RELEASE)
        set_target_properties(PCRE::PCRE PROPERTIES
                                         IMPORTED_CONFIGURATIONS RELEASE
                                         IMPORTED_LOCATION "${PCRE_LIBRARY_RELEASE}")
      endif()
      if(PCRE_LIBRARY_DEBUG)
        set_target_properties(PCRE::PCRE PROPERTIES
                                         IMPORTED_CONFIGURATIONS DEBUG
                                         IMPORTED_LOCATION "${PCRE_LIBRARY_DEBUG}")
      endif()
      set_target_properties(PCRE::PCRE PROPERTIES
                                       INTERFACE_INCLUDE_DIRECTORIES "${PCRE_INCLUDE_DIR}")
      if(WIN32)
        set_target_properties(PCRE::PCRE PROPERTIES
                                         INTERFACE_COMPILE_DEFINITIONS PCRE_STATIC=1)
      endif()

    endif()
    if(NOT TARGET PCRE::PCRECPP)
      add_library(PCRE::PCRECPP UNKNOWN IMPORTED)
      if(PCRECPP_LIBRARY_RELEASE)
        set_target_properties(PCRE::PCRECPP PROPERTIES
                                            IMPORTED_CONFIGURATIONS RELEASE
                                            IMPORTED_LOCATION "${PCRECPP_LIBRARY_RELEASE}")
      endif()
      if(PCRECPP_LIBRARY_DEBUG)
        set_target_properties(PCRE::PCRECPP PROPERTIES
                                            IMPORTED_CONFIGURATIONS DEBUG
                                            IMPORTED_LOCATION "${PCRECPP_LIBRARY_DEBUG}")
      endif()
      set_target_properties(PCRE::PCRECPP PROPERTIES
                                          INTERFACE_LINK_LIBRARIES PCRE::PCRE)
    endif()
  endif()
endif()

mark_as_advanced(PCRE_INCLUDE_DIR PCRECPP_LIBRARY PCRE_LIBRARY)
