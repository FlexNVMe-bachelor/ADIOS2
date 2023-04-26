#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# Findflan
# -----------
#
# Try to find the flan library
#
# This module defines the following variables:
#
#   FLAN_FOUND        - System has flan
#   FLAN_INCLUDE_DIR  - The flan include directory
#   FLAN_LIBRARY      - Link this to use flan
#
# You can also set the following variable to help guide the search:
#   FLAN_ROOT - The install prefix for FLAN containing the
#                     include and lib folders
#                     Note: this can be set as a CMake variable or an
#                           environment variable.  If specified as a CMake
#                           variable, it will override any setting specified
#                           as an environment variable.
if((NOT FLAN_ROOT) AND (NOT (ENV{FLAN_ROOT} STREQUAL "")))
  set(FLAN_ROOT "$ENV{FLAN_ROOT}")
endif()

set(FLAN_LIB_DIR ${FLAN_ROOT}/usr/local/lib/x86_64-linux-gnu)
if(FLAN_ROOT)
  set(FLAN_INCLUDE_OPTS HINTS ${FLAN_ROOT} ${FLAN_ROOT}/usr/local/include)
  set(FLAN_LIBRARY_OPTS HINTS ${FLAN_ROOT} ${FLAN_LIB_DIR})
endif()

find_path(FLAN_INCLUDE_DIR flan.h ${FLAN_INCLUDE_OPTS})
find_library(FLAN_LIBRARY libflan.so ${FLAN_LIBRARY_OPTS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(flan
  FOUND_VAR FLAN_FOUND
  REQUIRED_VARS FLAN_LIBRARY FLAN_INCLUDE_DIR
)

if(FLAN_FOUND)
  set(target flan::flan)
  if(NOT TARGET ${target})
    add_library(${target} SHARED IMPORTED)
    if(FLAN_INCLUDE_DIR)
      set_target_properties(${target} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FLAN_INCLUDE_DIR}")
    endif()

    if(FLAN_LIBRARY)
      set_target_properties(${target} PROPERTIES
        IMPORTED_LOCATION "${FLAN_LIBRARY}"
        IMPORTED_LINK_INTERFACE_LIBRARIES flan::xnme
        INTERFACE_LINK_DIRECTORIES "${FLAN_LIB_DIR}"
      )
    endif()

    target_link_libraries(${target} PUBLIC INTERFACE flan::xnvme)
  endif()
	
  message(STATUS "flan library \"${FLAN_LIBRARY}\"")
endif()
