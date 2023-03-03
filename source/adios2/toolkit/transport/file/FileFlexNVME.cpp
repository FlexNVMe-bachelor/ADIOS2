/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDescriptor.cpp file I/O using FlexNVME I/O library
 *
 *  Created on: Oct 6, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "FileFlexNVME.h"
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

FileFlexNVME::FileFlexNVME(helper::Comm const &comm)
: Transport("File", "FlexNVME", comm)
{
}

FileFlexNVME::~FileFlexNVME() noexcept { Close(); }

// TODO(adbo): s:
//  - Async open/close?
void FileFlexNVME::Open(const std::string &name, const Mode openMode,
                        const bool /*async*/, const bool /*directio*/)
{
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

auto FileFlexNVME::ErrnoErrMsg() const -> std::string
{
    return std::string(": errno = " + std::to_string(errno) + ": " +
                       strerror(errno));
}

void FileFlexNVME::OpenChain(const std::string &name, Mode openMode,
                             const helper::Comm &chainComm, const bool async,
                             const bool directio)
{
}

void FileFlexNVME::Write(const char *buffer, size_t size, size_t start) {}

#ifdef REALLY_WANT_WRITEV
void FileFlexNVME::WriteV(const core::iovec *iov, const int iovcnt,
                          size_t start)
{
}
#endif

void FileFlexNVME::Read(char *buffer, size_t size, size_t start) {}

size_t FileFlexNVME::GetSize() {}

void FileFlexNVME::Flush() {}

void FileFlexNVME::Close()
{
    if (m_IsOpen)
    {
        flan_close(flanh);
        m_IsOpen = false;
    }
}

void FileFlexNVME::Delete() {}

void FileFlexNVME::Seek(const size_t start) {}

void FileFlexNVME::Truncate(const size_t length) {}

void FileFlexNVME::MkDir(const std::string &fileName) {}

} // end namespace transport
} // end namespace adios2
