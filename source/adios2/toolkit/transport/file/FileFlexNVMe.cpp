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

#include <cstdio>  // remove
#include <cstring> // strerror
#include <errno.h> // errno
#include <fcntl.h> // open
#include <flan.h>
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
    std::cout << "Constructor was called\n";
}

FileFlexNVMe::~FileFlexNVMe() noexcept { Close(); }

// TODO(adbo): s:
//  - Async open/close?
void FileFlexNVMe::Open(const std::string &name, const Mode openMode,
                        const bool /*async*/, const bool /*directio*/)
{
    std::cout << "Open was called\n";
    m_Name = name;
    m_OpenMode = openMode;

    switch (m_OpenMode)
    {
    case Mode::Write:
    case Mode::Read:
        break;

    // TODO(adbo): more open modes?
    default:
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileFlexNVMe", "Open",
            "unknown open mode " + m_Name + " in call to FlexNVMe open");
    }

    ProfilerStart("open");
    struct fla_pool_create_arg pool_arg = {
        .flags = 0,
        .name = const_cast<char *>(name.c_str()),
        .name_len = static_cast<int>(name.length() + 1),
        .obj_nlb = 0, // will get set by flan_init
        .strp_nobjs = 0,
        .strp_nbytes = 0};

    uint64_t obj_size = 4096;
    char *device_uri = "/dev/loop11";

    printf("Opening flan\n");
    if (flan_init(device_uri, nullptr, &pool_arg, obj_size, &flanh))
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileFlexNVMe", "Open",
            "failed to initialise flan in call to FlexNVMe open: " +
                ErrnoErrMsg());
    }
    printf("Initialised flan\n");

    m_IsOpen = true;

    ProfilerStop("open");
}

auto FileFlexNVMe::ErrnoErrMsg() const -> std::string
{
    return std::string(": errno = " + std::to_string(errno) + ": " +
                       strerror(errno));
}

void FileFlexNVMe::OpenChain(const std::string &name, Mode openMode,
                             const helper::Comm &chainComm, const bool async,
                             const bool directio)
{
}

void FileFlexNVMe::Write(const char *buffer, size_t size, size_t start) {}

#ifdef REALLY_WANT_WRITEV
void FileFlexNVMe::WriteV(const core::iovec *iov, const int iovcnt,
                          size_t start)
{
}
#endif

void FileFlexNVMe::Read(char *buffer, size_t size, size_t start) {}

size_t FileFlexNVMe::GetSize() {}

void FileFlexNVMe::Flush() {}

void FileFlexNVMe::Close()
{
    if (m_IsOpen)
    {
        flan_close(flanh);
        m_IsOpen = false;
    }
}

void FileFlexNVMe::Delete() {}

void FileFlexNVMe::SeekToEnd(){};

void FileFlexNVMe::SeekToBegin(){};

void FileFlexNVMe::Seek(const size_t start) {}

void FileFlexNVMe::Truncate(const size_t length) {}

void FileFlexNVMe::MkDir(const std::string &fileName) {}

} // end namespace transport
} // end namespace adios2
