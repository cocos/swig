cmake_minimum_required (VERSION 3.15)

if (NOT DEFINED CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
endif ()

cmake_policy (SET CMP0091 NEW)

project (swig)

# set c++ standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (POLICY CMP0074)
  cmake_policy (SET CMP0074 NEW)
endif()

file (STRINGS configure.ac line LIMIT_COUNT 1 REGEX "AC_INIT\\(.*\\)" )
if (line MATCHES "AC_INIT\\(\\[(.*)\\],[ \t]*\\[(.*)\\],[ \t]*\\[(.*)\\]\\)" )
  set (SWIG_VERSION ${CMAKE_MATCH_2})
  set (PACKAGE_BUGREPORT ${CMAKE_MATCH_3})
else ()
  message (SEND_ERROR "Could not parse version from configure.ac")
endif ()

set (SWIG_ROOT ${PROJECT_SOURCE_DIR})

set (SWIG_LIB share/swig/${SWIG_VERSION})

# Project wide configuration variables
# ------------------------------------

set (SWIG_SOURCE_DIR ${SWIG_ROOT}/Source CACHE INTERNAL "Path of swig sources" FORCE)

set (PACKAGE_NAME swig)
set (PACKAGE_VERSION ${SWIG_VERSION})

# Configure
# ---------

list (APPEND CMAKE_MODULE_PATH ${SWIG_ROOT}/Tools/cmake)

include (CheckIncludeFiles)
include (CheckIncludeFile)
include (CheckIncludeFileCXX)
include (CheckTypeSize)
include (CheckSymbolExists)
include (CheckFunctionExists)
include (CheckLibraryExists)
include (CheckCSourceCompiles)

# HACK: didn't get the bool check working for Visual Studio 2008
if (MSVC)
  set(HAVE_BOOL 1)
else()
  set (CMAKE_EXTRA_INCLUDE_FILES stdbool.h)
  check_type_size ("bool" HAVE_BOOL)
  set (CMAKE_EXTRA_INCLUDE_FILES)
endif()

check_include_file ("inttypes.h" HAVE_INTTYPES_H)
check_include_file ("stddef.h" HAVE_STDDEF_H)
check_include_file ("stdint.h" HAVE_STDINT_H)
check_include_file ("stdio.h" HAVE_STDIO_H)
check_include_file ("stdlib.h" HAVE_STDLIB_H)
check_include_file ("string.h" HAVE_STRING_H)
check_include_file ("strings.h" HAVE_STRINGS_H)
check_include_file ("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file ("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file ("unistd.h" HAVE_UNISTD_H)
check_include_files ("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)

check_include_file_cxx ("boost/shared_ptr.hpp" HAVE_BOOST)
check_library_exists (dl dlopen "" HAVE_LIBDL)
check_function_exists (popen HAVE_POPEN)

if (MSVC)
  set (CMAKE_CXX_FLAGS "/EHsc ${CMAKE_CXX_FLAGS}")
endif ()

option (WITH_PCRE "Enable PCRE" ON)
if (WITH_PCRE)
  find_package (PCRE2 REQUIRED)
  set (HAVE_PCRE 1)
  include_directories (${PCRE2_INCLUDE_DIRS})
endif()

if (WIN32)
  file (TO_NATIVE_PATH ${CMAKE_INSTALL_PREFIX}/${SWIG_LIB} SWIG_LIB_WIN_UNIX)
  string (REGEX REPLACE "\\\\" "\\\\\\\\" SWIG_LIB_WIN_UNIX "${SWIG_LIB_WIN_UNIX}")
endif ()
configure_file (${SWIG_ROOT}/Tools/cmake/swigconfig.h.in
                ${CMAKE_CURRENT_BINARY_DIR}/Source/Include/swigconfig.h)

find_package (BISON REQUIRED)


# Compiler flags
# --------------

include_directories (
    ${SWIG_SOURCE_DIR}/CParse
    ${SWIG_SOURCE_DIR}/Include
    ${SWIG_SOURCE_DIR}/DOH
    ${SWIG_SOURCE_DIR}/Swig
    ${SWIG_SOURCE_DIR}/Preprocessor
    ${SWIG_SOURCE_DIR}/Modules
    ${PROJECT_BINARY_DIR}/Source/Include
    ${PROJECT_BINARY_DIR}/Source/CParse
    ${PROJECT_SOURCE_DIR}/Source/Doxygen
)

# generate the parser source code (depends on bison)
file (MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/Source/CParse)

BISON_TARGET (swig_parser
    ${SWIG_SOURCE_DIR}/CParse/parser.y
    ${PROJECT_BINARY_DIR}/Source/CParse/parser.c
)

# generate swigwarn.swg
file (READ ${SWIG_SOURCE_DIR}/Include/swigwarn.h SWIG_WARN_H)
string (REGEX REPLACE "#define WARN([^ \\t]*)[ \\t]*([0-9]+)" "%define SWIGWARN\\1 \\2 %enddef" SWIG_WARN_SWG ${SWIG_WARN_H})
file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/swigwarn.swg ${SWIG_WARN_SWG})
set_property (SOURCE ${CMAKE_CURRENT_BINARY_DIR}/swigwarn.swg PROPERTY GENERATED 1)

# install lib
install (DIRECTORY ${SWIG_ROOT}/Lib/ DESTINATION ${SWIG_LIB})
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/swigwarn.swg DESTINATION ${SWIG_LIB})

# sources
# ---------
file (GLOB DOH_SOURCES ${SWIG_SOURCE_DIR}/DOH/*.c ${SWIG_SOURCE_DIR}/DOH/*.h)
file (GLOB CPARSE_SOURCES ${SWIG_SOURCE_DIR}/CParse/*.c ${SWIG_SOURCE_DIR}/CParse/*.h)
list (APPEND CPARSE_SOURCES)
file (GLOB PREPROCESSOR_SOURCES ${SWIG_SOURCE_DIR}/Preprocessor/*.c ${SWIG_SOURCE_DIR}/Preprocessor/*.h)
file (GLOB CORE_SOURCES ${SWIG_SOURCE_DIR}/Swig/*.c ${SWIG_SOURCE_DIR}/Swig/*.h)
file (GLOB DOXYGEN_SOURCES ${SWIG_SOURCE_DIR}/Doxygen/*.cxx ${SWIG_SOURCE_DIR}/Doxygen/*.h)

# We only needs cocos backend
# file (GLOB MODULES_SOURCES ${SWIG_SOURCE_DIR}/Modules/*.cxx ${SWIG_SOURCE_DIR}/Modules/*.h)

set (MODULES_SOURCES 
  ${SWIG_SOURCE_DIR}/Modules/cocos.cxx
  ${SWIG_SOURCE_DIR}/Modules/contract.cxx
  ${SWIG_SOURCE_DIR}/Modules/emit.cxx
  ${SWIG_SOURCE_DIR}/Modules/main.cxx
  ${SWIG_SOURCE_DIR}/Modules/lang.cxx
  ${SWIG_SOURCE_DIR}/Modules/overload.cxx
  ${SWIG_SOURCE_DIR}/Modules/nested.cxx
  ${SWIG_SOURCE_DIR}/Modules/typepass.cxx
  ${SWIG_SOURCE_DIR}/Modules/interface.cxx
  ${SWIG_SOURCE_DIR}/Modules/utils.cxx
  ${SWIG_SOURCE_DIR}/Modules/xml.cxx
  ${SWIG_SOURCE_DIR}/Modules/allocate.cxx
  ${SWIG_SOURCE_DIR}/Modules/swigmain.cxx
  ${SWIG_SOURCE_DIR}/Modules/swigmod.h
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX "Source Files" FILES ${DOH_SOURCES})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX "Source Files" FILES ${CPARSE_SOURCES})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX "Source Files" FILES ${PREPROCESSOR_SOURCES})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX "Source Files" FILES ${CORE_SOURCES})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX "Source Files" FILES ${DOXYGEN_SOURCES})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX "Source Files" FILES ${MODULES_SOURCES})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX "Source Files" FILES ${MODULES_SOURCES})

add_executable (swig
  ${CPARSE_SOURCES}
  ${DOH_SOURCES}
  ${DOXYGEN_SOURCES}
  ${MODULES_SOURCES}
  ${CORE_SOURCES}
  ${PREPROCESSOR_SOURCES}
  ${PROJECT_BINARY_DIR}/Source/Include/swigconfig.h
  ${SWIG_SOURCE_DIR}/Include/swigwarn.h
  ${PROJECT_BINARY_DIR}/Source/CParse/parser.c
  ${PROJECT_BINARY_DIR}/Source/CParse/parser.h
)

if (MSVC)
set_property(TARGET swig PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

if (PCRE2_FOUND)
  target_link_libraries (swig ${PCRE2_LIBRARIES} -static)
else ()
  target_link_libraries (swig -static)
endif ()

install (TARGETS swig DESTINATION bin)

# 'make package-source' creates tarballs
set (CPACK_PACKAGE_NAME ${PACKAGE_NAME})
set (CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
set (CPACK_SOURCE_IGNORE_FILES "/.git;/build;.*~;${CPACK_SOURCE_IGNORE_FILES}")
set (CPACK_SOURCE_PACKAGE_FILE_NAME ${PACKAGE_NAME}-${PACKAGE_VERSION})
include (CPack)

# few tests
enable_testing ()
add_test (NAME cmd_version COMMAND swig -version)
add_test (NAME cmd_pcreversion COMMAND swig -pcreversion)
add_test (NAME cmd_swiglib COMMAND swig -swiglib)
add_test (NAME cmd_external_runtime COMMAND swig -external-runtime ext_rt.h)
set_tests_properties(cmd_external_runtime PROPERTIES ENVIRONMENT "SWIG_LIB=${PROJECT_SOURCE_DIR}/Lib")

