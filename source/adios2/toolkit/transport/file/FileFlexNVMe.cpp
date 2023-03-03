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
#include <iostream>

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

FileFlexNVMe::~FileFlexNVMe() {}

void FileFlexNVMe::Open(const std::string &name, const Mode openMode,
                        const bool async, const bool directio)
{
    m_baseName = name;
}

void FileFlexNVMe::OpenChain(const std::string &name, Mode openMode,
                             const helper::Comm &chainComm, const bool async,
                             const bool directio)
{
    m_baseName = name;
}

void FileFlexNVMe::Write(const char *buffer, size_t size, size_t start)
{
    // Set the chunk size since it's not been set before.
    if (m_chunkSize == 0)
    {
        m_chunkSize = size;
    }
    else if (m_chunkSize > size)
    {
        std::cerr
            << "Warning: Dynamic chunk sizes are not supported by "
               "FlexNVMe. It will be saved, but will be space inefficient\n";
    }
    else if (m_chunkSize < size)
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileFlexNVMe", "Write",
            "Chunksize cannot be bigger than the first chunk written");
    }

    std::string objectName = CreateChunkName();
}

#ifdef REALLY_WANT_WRITEV
void FileFlexNVMe::WriteV(const core::iovec *iov, const int iovcnt,
                          size_t start)
{
}
#endif

void FileFlexNVMe::Read(char *buffer, size_t size, size_t start) {}

size_t FileFlexNVMe::GetSize() {}

void FileFlexNVMe::Flush() {}

void FileFlexNVMe::Close() {}

void FileFlexNVMe::Delete() {}

void FileFlexNVMe::SeekToEnd() {}

void FileFlexNVMe::SeekToBegin() {}

void FileFlexNVMe::Seek(const size_t start) {}

void FileFlexNVMe::Truncate(const size_t length) {}

void FileFlexNVMe::MkDir(const std::string &fileName) {}

std::string FileFlexNVMe::CreateChunkName()
{
    if (m_baseName == "")
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileFlexNVMe", "CreateChunkName",
            "Empty filenames are not supported");
    }

    std::string base = m_baseName;
    base += "#" + std::to_string(m_chunkWrites);
    m_chunkWrites += 1;

    return base;
}

} // end namespace transport
} // end namespace adios2
