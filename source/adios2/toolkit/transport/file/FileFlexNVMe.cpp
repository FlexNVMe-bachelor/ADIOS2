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
#include "adios2/common/ADIOSTypes.h"
#include "adios2/helper/adiosLog.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <string>

#include <flan.h>

namespace adios2
{
namespace transport
{

int FileFlexNVMe::refCount = 0;
struct flan_handle *FileFlexNVMe::flanh = nullptr;

FileFlexNVMe::FileFlexNVMe(helper::Comm const &comm)
: Transport("File", "FlexNVMe", comm)
{
    FileFlexNVMe::refCount++;
}

FileFlexNVMe::~FileFlexNVMe() noexcept
{
    FileFlexNVMe::refCount--;
    Close();
}

// TODO(adbo):
//  - Async open/close?
//  - Remove printf/cout
void FileFlexNVMe::Open(const std::string &name, const Mode openMode,
                        const bool /*async*/, const bool /*directio*/)
{
    m_Name = name;
    m_OpenMode = openMode;
    m_baseName = NormalisedObjectName(const_cast<std::string &>(name));

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

    // TODO(adbo): take as parameter
    InitFlan("tmp hardcoded pool name");
    m_IsOpen = true;

    ProfilerStop("open");
}

void FileFlexNVMe::InitFlan(const std::string &pool_name)
{
    if (FileFlexNVMe::flanh != nullptr)
    {
        return;
    }

    struct fla_pool_create_arg pool_arg = {
        .flags = 0,
        .name = const_cast<char *>(pool_name.c_str()),
        .name_len = static_cast<int>(pool_name.length() + 1),
        .obj_nlb = 0, // will get set by flan_init
        .strp_nobjs = 0,
        .strp_nbytes = 0};

    // TODO(adbo): take as argument
    uint64_t obj_size = 4096;
    char *device_uri = "/dev/loop11";

    if (flan_init(device_uri, nullptr, &pool_arg, obj_size,
                  &FileFlexNVMe::flanh))
    {
        // We have to reset the address to null because flan_init can change its
        // value even when it fails.
        FileFlexNVMe::flanh = nullptr;

        std::string err =
            "failed to initialise flan in call to FlexNVMe open: " +
            ErrnoErrMsg();

        if (errno == 22)
        {
            helper::Throw<std::ios_base::failure>(
                "Toolkit", "transport::file::FileFlexNVMe", "Open",
                err + ". You may need to mkfs.flexalloc the storage device.");
        }

        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileFlexNVMe", "Open", err);
    }
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
    m_baseName = name;
}

// TODO(adbo):
//   - Set chunk size via parameters or make them static, because
//     currently they are not shared across transport instances
//   - Use the start parameter?
void FileFlexNVMe::Write(const char *buffer, size_t size, size_t start)
{
    // Set the chunk size since it's not been set before.
    if (m_chunkSize == 0)
    {
        m_chunkSize = size;
    }
    else if (m_chunkSize > size)
    {
        helper::Log("Toolkit", "transport::file::FileFlexNVMe", "Write",
                    "Dynamic chunk sizes are not supported by FlexNVMe. It "
                    "will be saved, but will be space inefficient",
                    helper::WARNING);
    }
    else if (m_chunkSize < size)
    {
        helper::Throw<std::invalid_argument>(
            "Toolkit", "transport::file::FileFlexNVMe", "Write",
            "Chunksize cannot be bigger than the first chunk written");
    }

    std::string objectName = CreateChunkName();

    uint64_t objectHandle = OpenFlanObject(objectName);

    if (flan_object_write(objectHandle,
                          static_cast<void *>(const_cast<char *>(buffer)), 0,
                          size, FileFlexNVMe::flanh))
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileFlexNVMe", "Write",
            "Failed to write chunk " + objectName);
    }
}

#ifdef REALLY_WANT_WRITEV
void FileFlexNVMe::WriteV(const core::iovec *iov, const int iovcnt,
                          size_t start)
{
}
#endif

void FileFlexNVMe::Read(char *buffer, size_t size, size_t start) {}

size_t FileFlexNVMe::GetSize()
{
    if (FileFlexNVMe::flanh == nullptr)
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileFlexNVMe", "GetSize",
            "GetSize called before Open");
    }

    // TODO(adbo): iterate over all chunks
    std::string objectName = TmpCreateChunkName(0);

    struct flan_oinfo *objectInfo =
        flan_find_oinfo(FileFlexNVMe::flanh, objectName.c_str(), nullptr);

    if (objectInfo == nullptr)
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileFlexNVMe", "GetSize",
            "Object '" + m_Name + "' not found (chunk: '" + objectName + "')");
    }

    return objectInfo->size;
}

void FileFlexNVMe::Flush() {}

void FileFlexNVMe::Close()
{
    if (FileFlexNVMe::refCount <= 0)
    {
        flan_close(FileFlexNVMe::flanh);
        FileFlexNVMe::flanh = nullptr;
    }

    m_IsOpen = false;
}

void FileFlexNVMe::Delete() {}

void FileFlexNVMe::SeekToEnd() {}

void FileFlexNVMe::SeekToBegin() {}

void FileFlexNVMe::Seek(const size_t start) {}

void FileFlexNVMe::Truncate(const size_t length) {}

void FileFlexNVMe::MkDir(const std::string &fileName) {}

std::string FileFlexNVMe::NormalisedObjectName(std::string &input)
{
    std::string normalised = input;
    std::replace(normalised.begin(), normalised.end(), '/', '_');
    return normalised;
}

std::string FileFlexNVMe::TmpCreateChunkName(size_t chunkNum)
{
    std::string base = m_baseName;
    base += "#" + std::to_string(chunkNum);
    return base;
}

std::string FileFlexNVMe::CreateChunkName()
{
    if (m_baseName == "")
    {
        helper::Throw<std::range_error>(
            "Toolkit", "transport::file::FileFlexNVMe", "CreateChunkName",
            "Empty filenames are not supported");
    }

    std::string objectName = TmpCreateChunkName(m_chunkWrites);
    m_chunkWrites += 1;

    return objectName;
}

auto FileFlexNVMe::OpenFlanObject(std::string &objectName) -> uint64_t
{
    uint64_t objectHandle = 0;
    if (flan_object_open(objectName.c_str(), FileFlexNVMe::flanh, &objectHandle,
                         FLAN_OPEN_FLAG_CREATE | FLAN_OPEN_FLAG_WRITE))
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileFlexNVMe", "Write",
            "Failed to open object " + objectName);
    }
    return objectHandle;
}

} // end namespace transport
} // end namespace adios2
