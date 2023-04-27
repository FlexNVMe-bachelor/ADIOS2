#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(File MPI_NONE "" Transports. "")

function(gtest_add_flexnvme_test testname)
  set(target Test.Transports.FileFlexNVMe${testname})
  
  message(STATUS "Checking if target ${target}")

  if(NOT TARGET ${target})
    message(STATUS "Making target ${target}")
    add_executable(${target} TestFileFlexNVMe${testname}.cpp disk/Loopback.cpp disk/BackingFile.cpp disk/mkfs.cpp disk/DiskTestClass.cpp)

    target_link_libraries(${target} adios2::cxx11 adios2_core adios2::thirdparty::gtest stdc++fs)
  endif()
endfunction()

if(ADIOS2_HAVE_FLAN AND ADIOS2_HAVE_FLEXALLOC)
  gtest_add_flexnvme_test(ChunkNaming)
  gtest_add_flexnvme_test(Open)
  gtest_add_flexnvme_test(Parameters)
  gtest_add_flexnvme_test(ReadWrite)
  gtest_add_flexnvme_test(GetSize)
  gtest_add_flexnvme_test(MatchingFilePOSIX)
  gtest_add_flexnvme_test(IntegrationLocalArrayExample)
endif()

