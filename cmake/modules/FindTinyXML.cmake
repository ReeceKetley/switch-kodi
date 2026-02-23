#.rst:
# FindTinyXML
# -----------
# Finds the TinyXML library
#
# This will define the following variables::
#
# TINYXML_FOUND - system has TinyXML
# TINYXML_INCLUDE_DIRS - the TinyXML include directory
# TINYXML_LIBRARIES - the TinyXML libraries
# TINYXML_DEFINITIONS - the TinyXML definitions
#
# and the following imported targets::
#
#   TinyXML::TinyXML   - The TinyXML library

if(ENABLE_INTERNAL_TINYXML)
  include(ExternalProject)

  file(STRINGS ${CMAKE_SOURCE_DIR}/tools/depends/target/tinyxml/Makefile VER REGEX "^[ ]*VERSION[ ]*=.+$")
  string(REGEX REPLACE "^[ ]*VERSION[ ]*=[ ]*" "" TINYXML_VERSION "${VER}")

  if(TINYXML_URL)
    get_filename_component(TINYXML_URL "${TINYXML_URL}" ABSOLUTE)
  else()
    set(TINYXML_URL http://mirrors.kodi.tv/build-deps/sources/tinyxml-${TINYXML_VERSION}.tar.gz)
  endif()
  if(VERBOSE)
    message(STATUS "TINYXML_URL: ${TINYXML_URL}")
  endif()

  set(TINYXML_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/tinyxml/lib/libtinyxml.a)
  set(TINYXML_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/tinyxml/include)
  set(TINYXML_HOST_TRIPLET ${ARCH})
  set(TINYXML_LD ${CMAKE_LINKER})
  if(CORE_PLATFORM_NAME STREQUAL switch)
    set(TINYXML_HOST_TRIPLET aarch64-none-elf)
    set(TINYXML_LD /opt/devkitpro/devkitA64/aarch64-none-elf/bin/ld)
  endif()

  ExternalProject_Add(tinyxml
                      URL ${TINYXML_URL}
                      DOWNLOAD_NAME tinyxml-${TINYXML_VERSION}.tar.gz
                      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
                      PREFIX ${CORE_BUILD_DIR}/tinyxml
                      BUILD_IN_SOURCE 1
                      CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env
                                        AR=${CMAKE_AR}
                                        RANLIB=${CMAKE_RANLIB}
                                        CC=${CMAKE_C_COMPILER}
                                        CXX=${CMAKE_CXX_COMPILER}
                                        LD=${TINYXML_LD}
                                        CXXLD=${TINYXML_LD}
                                        <SOURCE_DIR>/configure
                                        --host=${TINYXML_HOST_TRIPLET}
                                        --prefix=<INSTALL_DIR>
                                        --disable-dependency-tracking
                                        --disable-shared
                                        --enable-static
                      BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} -j1 -C <SOURCE_DIR>/src
                      INSTALL_COMMAND ${CMAKE_MAKE_PROGRAM} -C <SOURCE_DIR>/src install
                      BUILD_BYPRODUCTS ${TINYXML_LIBRARY})
  ExternalProject_Add_Step(tinyxml autoreconf
                           DEPENDEES download update patch
                           DEPENDERS configure
                           COMMAND rm -f config.status
                           COMMAND autoreconf -vif
                           WORKING_DIRECTORY <SOURCE_DIR>)
  set_target_properties(tinyxml PROPERTIES FOLDER "External Projects")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(TinyXML
                                    REQUIRED_VARS TINYXML_LIBRARY TINYXML_INCLUDE_DIR
                                    VERSION_VAR TINYXML_VERSION)

  set(TINYXML_LIBRARIES ${TINYXML_LIBRARY})
  set(TINYXML_INCLUDE_DIRS ${TINYXML_INCLUDE_DIR})
else()
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_TINYXML tinyxml QUIET)
  endif()

  find_path(TINYXML_INCLUDE_DIR tinyxml.h
                                PATH_SUFFIXES tinyxml
                                PATHS ${PC_TINYXML_INCLUDEDIR})
  find_library(TINYXML_LIBRARY_RELEASE NAMES tinyxml tinyxmlSTL
                                       PATH_SUFFIXES tinyxml
                                       PATHS ${PC_TINYXML_LIBDIR})
  find_library(TINYXML_LIBRARY_DEBUG NAMES tinyxmld tinyxmlSTLd
                                     PATH_SUFFIXES tinyxml
                                     PATHS ${PC_TINYXML_LIBDIR})
  set(TINYXML_VERSION ${PC_TINYXML_VERSION})

  include(SelectLibraryConfigurations)
  select_library_configurations(TINYXML)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(TinyXML
                                    REQUIRED_VARS TINYXML_LIBRARY TINYXML_INCLUDE_DIR
                                    VERSION_VAR TINYXML_VERSION)

  if(TINYXML_FOUND)
    set(TINYXML_LIBRARIES ${TINYXML_LIBRARY})
    set(TINYXML_INCLUDE_DIRS ${TINYXML_INCLUDE_DIR})
    if(WIN32)
      set(TINYXML_DEFINITIONS -DTIXML_USE_STL=1)
    endif()

    if(NOT TARGET TinyXML::TinyXML)
      add_library(TinyXML::TinyXML UNKNOWN IMPORTED)
      if(TINYXML_LIBRARY_RELEASE)
        set_target_properties(TinyXML::TinyXML PROPERTIES
                                               IMPORTED_CONFIGURATIONS RELEASE
                                               IMPORTED_LOCATION "${TINYXML_LIBRARY_RELEASE}")
      endif()
      if(TINYXML_LIBRARY_DEBUG)
        set_target_properties(TinyXML::TinyXML PROPERTIES
                                               IMPORTED_CONFIGURATIONS DEBUG
                                               IMPORTED_LOCATION "${TINYXML_LIBRARY_DEBUG}")
      endif()
      set_target_properties(TinyXML::TinyXML PROPERTIES
                                             INTERFACE_INCLUDE_DIRECTORIES "${TINYXML_INCLUDE_DIR}")
      if(WIN32)
        set_target_properties(TinyXML::TinyXML PROPERTIES
                                               INTERFACE_COMPILE_DEFINITIONS TIXML_USE_STL=1)
      endif()
    endif()
  endif()
endif()

mark_as_advanced(TINYXML_INCLUDE_DIR TINYXML_LIBRARY)
