#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# Findflexalloc
# -----------
#
# Try to find the flexalloc library
#
# This module defines the following variables:
#
#   FLEXALLOC_FOUND        - System has flexalloc
#   FLEXALLOC_INCLUDE_DIR  - The flexalloc include directory
#   FLEXALLOC_LIBRARY      - Link this to use flexalloc
#
# You can also set the following variable to help guide the search:
#   FLEXALLOC_ROOT - The install prefix for flexalloc containing the
#                     include and lib folders
#                     Note: this can be set as a CMake variable or an
#                           environment variable.  If specified as a CMake
#                           variable, it will override any setting specified
#                           as an environment variable.
if((NOT FLEXALLOC_ROOT) AND (NOT (ENV{FLEXALLOC_ROOT} STREQUAL "")))
  set(FLEXALLOC_ROOT "$ENV{FLEXALLOC_ROOT}")
endif()

if(FLEXALLOC_ROOT)
  # This is the behavior of meson install with custom install directory
  set(FLEXALLOC_INCLUDE_OPTS HINTS ${FLEXALLOC_ROOT} ${FLEXALLOC_ROOT}/usr/local/include)
  set(FLEXALLOC_LIBRARY_OPTS HINTS ${FLEXALLOC_ROOT} ${FLEXALLOC_ROOT}/usr/local/lib/x86_64-linux-gnu)
endif()

find_path(
  FLEXALLOC_INCLUDE_DIR 
  NAMES libflexalloc.h flexalloc_mm.h
  ${FLEXALLOC_INCLUDE_OPTS}
)
find_library(FLEXALLOC_LIBRARY libflexalloc.so ${FLEXALLOC_LIBRARY_OPTS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(flexalloc
  FOUND_VAR FLEXALLOC_FOUND
  REQUIRED_VARS FLEXALLOC_LIBRARY FLEXALLOC_INCLUDE_DIR
)

if(FLEXALLOC_FOUND)
  set(FLEXALLOC_INCLUDE_DIR ${FLEXALLOC_INCLUDE_DIR})
  set(FLEXALLOC_LIBRARY ${FLEXALLOC_LIBRARY})
  message(STATUS "FLEXALLOC library \"${FLEXALLOC_LIBRARY}\"")
  message(STATUS "FLEXALLOC include \"${FLEXALLOC_INCLUDE_DIR}\"")
endif()
