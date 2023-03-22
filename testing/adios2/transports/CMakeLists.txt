#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(File MPI_NONE "" Transports. "")

if(ADIOS2_HAVE_FLAN AND ADIOS2_HAVE_FLEXALLOC)
  gtest_add_tests_helper(FileFlexNVMeChunkNaming MPI_NONE "" Transports. "")
  gtest_add_tests_helper(FileFlexNVMeChunkSize MPI_NONE "" Transports. "")

  add_executable(Test.Transports.FileFlexNVMeOpen TestFileFlexNVMeOpen.cpp disk/Loopback.cpp disk/BackingFile.cpp disk/mkfs.cpp)
  target_include_directories(Test.Transports.FileFlexNVMeOpen PRIVATE ${FLEXALLOC_INCLUDE_DIR})
  target_link_libraries(Test.Transports.FileFlexNVMeOpen flexalloc adios2::cxx11 adios2_core adios2::thirdparty::gtest stdc++fs)
endif()

