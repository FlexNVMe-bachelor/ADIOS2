#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# Findxnvme
# -----------
#
# Try to find the xnvme library
#
# This module defines the following variables:
#
#   XNVME_FOUND        - System has xnvme
#   XNVME_INCLUDE_DIR  - The xnvme include directory
#   XNVME_LIBRARY      - Link this to use xnvme
#
# You can also set the following variable to help guide the search:
#   XNVME_ROOT - The install prefix for xnvme containing the
#                     include and lib folders
#                     Note: this can be set as a CMake variable or an
#                           environment variable.  If specified as a CMake
#                           variable, it will override any setting specified
#                           as an environment variable.
#
#   XNVME_PKGCONFIG_IGNORE - Dont search with pkgconfig

include(FindPackageHandleStandardArgs)

if((NOT XNVME_ROOT) AND (NOT (ENV{XNVME_ROOT} STREQUAL "")))
  set(XNVME_ROOT "$ENV{XNVME_ROOT}")
endif()

if(NOT XNVME_PKGCONFIG_IGNORE)
  message(STATUS "PKGCONFIG FOUND: ${PKG_CONFIG_FOUND}")
  find_package(PkgConfig)
  message(STATUS "PKGCONFIG FOUND: ${PKG_CONFIG_FOUND}")

  if(PKG_CONFIG_FOUND)
    set(_XNVME_CMAKE_PREFIX_PATH_BACKUP ${CMAKE_PREFIX_PATH})

    if(XNVME_ROOT)
      list(INSERT CMAKE_PREFIX_PATH 0 "${XNVME_ROOT}")
    endif()

    message(STATUS "CMAKE_PREFIX: ${CMAKE_PREFIX_PATH}")

    # Honestly not sure what this does
    set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
    set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)

    # PC=PkgConfig
    pkg_check_modules(PC_XNVME xnvme)

    # Restore
    set(CMAKE_PREFIX_PATH ${_XNVME_CMAKE_PREFIX_PATH_BACKUP})
    unset(_XNVME_CMAKE_PREFIX_PATH_BACKUP)


    message(STATUS "XNVME_INCLUDE_DIR ${PC_XNVME_INCLUDE_DIRS}")
    message(STATUS "XNVME_STATIC_INCLUDE_DIR ${PC_XNVME_STATIC_INCLUDE_DIRS}")
    message(STATUS "XNVME_LIBRARY ${PC_XNVME_LINK_LIBRARIES}")
    message(STATUS "XNVME_STATIC_LIBRARY ${PC_XNVME_STATIC_LINK_LIBRARIES}")
    message(STATUS "XNVME_LIBRARY_DIR ${PC_XNVME_LIBRARY_DIRS}")
    message(STATUS "XNVME_STATIC_LIBRARY_DIR ${PC_XNVME_STATIC_LIBRARY_DIRS}")
    message(STATUS "XNVME_FOUND ${PC_XNVME_FOUND}")
    message(STATUS "XNVME_VERSION ${PC_XNVME_VERSION}")

    if(PC_XNVME_FOUND)
      set(XNVME_INCLUDE_DIR ${PC_XNVME_INCLUDE_DIRS})
      set(XNVME_LIBRARY ${PC_XNVME_LINK_LIBRARIES})
      # set(XNVME_LIBRARY_DIR ${PC_XNVME_LIBRARY_DIRS})
      set(XNVME_FOUND ${PC_XNVME_FOUND})
      set(XNVME_VERSION ${PC_XNVME_VERSION})
    endif()
  endif()

  find_package_handle_standard_args(xnvme
    FOUND_VAR XNVME_FOUND
    VERSION_VAR XNVME_VERSION
    REQUIRED_VARS XNVME_LIBRARY)
else()
  if(XNVME_ROOT)
    # This is the behavior of meson install with custom install directory
    set(XNVME_INCLUDE_OPTS HINTS ${XNVME_ROOT} ${XNVME_ROOT}/usr/local/include)
    set(XNVME_LIBRARY_OPTS HINTS ${XNVME_ROOT} ${XNVME_ROOT}/usr/local/lib/x86_64-linux-gnu)
  endif()

  find_path(
    XNVME_INCLUDE_DIR 
    NAMES libxnvme.h
    ${XNVME_INCLUDE_OPTS}
  )
  find_library(XNVME_LIBRARY libxnvme.so ${XNVME_LIBRARY_OPTS})
  
  find_package_handle_standard_args(xnvme
    FOUND_VAR XNVME_FOUND
    REQUIRED_VARS XNVME_LIBRARY XNVME_INCLUDE_DIR
  )
endif()

if(XNVME_FOUND)
  set(target flan::xnvme)

  if(NOT TARGET ${target})
    add_library(${target} SHARED IMPORTED)
    if(XNVME_INCLUDE_DIR)
      set_target_properties(${target} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${XNVME_INCLUDE_DIR}")
    endif()

    if(XNVME_LIBRARY)
      set_target_properties(${target} PROPERTIES
        IMPORTED_LOCATION "${XNVME_LIBRARY}"
        INTERFACE_LINK_DIRECTORIES "${XNVME_LIBRARY_DIR}"
      )
    endif()
    message(STATUS "XNVME library \"${XNVME_LIBRARY}\"")
    message(STATUS "XNVME include \"${XNVME_INCLUDE_DIR}\"")
  endif()
endif()
