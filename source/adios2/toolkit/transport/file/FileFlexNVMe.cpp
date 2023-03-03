/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDescriptor.cpp file I/O using FlexNVMe I/O library
 *
 *  Created on: Oct 6, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "FileFlexNVMe.h"
#include "adios2/helper/adiosLog.h"

#ifdef ADIOS2_HAVE_O_DIRECT
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include <cstdio>      // remove
#include <cstring>     // strerror
#include <errno.h>     // errno
#include <fcntl.h>     // open
#include <sys/stat.h>  // open, fstat
#include <sys/types.h> // open
#include <unistd.h>    // write, close, ftruncate

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> //std::ios_base::failure
/// \endcond

namespace adios2
{
namespace transport
{

FileFlexNVMe::FileFlexNVMe(helper::Comm const &comm)
: Transport("File", "FlexNVMe", comm)
{
}

FileFlexNVME::~FileFlexNVME() {}

void FileFlexNVME::WaitForOpen() {}

void FileFlexNVMe::Open(const std::string &name, const Mode openMode,
                        const bool async, const bool directio)
{
}

void FileFlexNVMe::OpenChain(const std::string &name, Mode openMode,
                             const helper::Comm &chainComm, const bool async,
                             const bool directio)
{
}

void FileFlexNVME::Write(const char *buffer, size_t size, size_t start) {}

#ifdef REALLY_WANT_WRITEV
void FileFlexNVMe::WriteV(const core::iovec *iov, const int iovcnt,
                          size_t start)
{
}
#endif

void FileFlexNVME::Read(char *buffer, size_t size, size_t start) {}

size_t FileFlexNVME::GetSize() {}

void FileFlexNVME::Flush() {}

void FileFlexNVME::Close() {}

void FileFlexNVME::Delete() {}

void FileFlexNVME::Seek(const size_t start) {}

void FileFlexNVME::Truncate(const size_t length) {}

void FileFlexNVMe::MkDir(const std::string &fileName) {}

} // end namespace transport
} // end namespace adios2
