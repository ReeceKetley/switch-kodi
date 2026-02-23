set(SOURCES emu_dummy.cpp)
if(NOT CORE_PLATFORM_NAME STREQUAL switch)
  list(APPEND SOURCES emu_msvcrt.cpp)
endif()

set(HEADERS emu_dummy.h
            emu_msvcrt.h)

core_add_library(dllexports)

if(TRUE)
  set(WRAPPER_OBJ ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/wrapper.dir/wrapper.c.o)
  add_custom_command(OUTPUT ${WRAPPER_OBJ}
                     COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/wrapper.dir
                     COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/wrapper.dir/compiler_depend.make
                     COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/wrapper.dir/compiler_depend.make
                     COMMAND ${CMAKE_C_COMPILER} -c -x c -o ${WRAPPER_OBJ} /dev/null)
  add_custom_target(wrapper DEPENDS ${WRAPPER_OBJ})

  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wrapper.def
                     COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/wrapper.def)
  add_custom_target(wrapper.def ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/wrapper.def)

  set_target_properties(wrapper PROPERTIES FOLDER "Build Utilities")
  set_target_properties(wrapper.def PROPERTIES FOLDER "Build Utilities")
  add_dependencies(wrapper.def wrapper)
elseif(APPLE)
  add_library(wrapper OBJECT wrapper.c)
  add_custom_target(wrapper.def ALL ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/wrapper_mach_alias wrapper.def)
  set_target_properties(wrapper PROPERTIES FOLDER "Build Utilities")
  set_target_properties(wrapper.def PROPERTIES FOLDER "Build Utilities")
  add_dependencies(wrapper.def wrapper)
elseif(NOT CORE_SYSTEM_NAME STREQUAL windows AND NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
  add_options(C ALL_BUILDS "-fPIC")
  add_library(wrapper OBJECT wrapper.c)

  add_custom_target(wrapper.def ALL ${CMAKE_NM} ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/wrapper.dir/wrapper.c.o | grep __wrap | awk '{ printf(\"%s \", \$\$3) }' | sed \"s/___wrap_/__wrap_/g\" | sed \"s/__wrap_/-Wl,-wrap,/g\" > wrapper.def && test -s wrapper.def)

  if(CORE_SYSTEM_NAME STREQUAL android)
    add_custom_command(TARGET wrapper.def COMMAND echo \"-L${DEPENDS_PATH}/lib/dummy-lib${APP_NAME_LC} -l${APP_NAME_LC}\" >> wrapper.def)
  endif()

  set_target_properties(wrapper PROPERTIES FOLDER "Build Utilities")
  set_target_properties(wrapper.def PROPERTIES FOLDER "Build Utilities")
  add_dependencies(wrapper.def wrapper)
endif()

