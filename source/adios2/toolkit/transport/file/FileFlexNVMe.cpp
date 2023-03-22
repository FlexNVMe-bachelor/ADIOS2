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
#include "adios2/helper/adiosString.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ios>
#include <iostream>
#include <stdexcept>

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
    m_baseName = name;

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

void FileFlexNVMe::SetParameters(const Params &params)
{
    std::string tmp = "";
    helper::SetParameterValue("device_url", params, tmp);
    std::cout << "Parameter value (device): " << m_deviceUrl << "\n";
    if (tmp.empty())
    {
        helper::Throw<std::invalid_argument>(
            "Toolkit", "transport::file::FileFlexNVMe", "SetParameters",
            "device_url parameter has not been set");
    }

    m_deviceUrl = const_cast<char *>(tmp.c_str());

    helper::SetParameterValue("pool_name", params, m_poolName);
    std::cout << "Parameter value (pool): " << m_poolName << "\n";
    if (m_poolName.empty())
    {
        helper::Throw<std::invalid_argument>(
            "Toolkit", "transport::file::FileFlexNVMe", "SetParameters",
            "pool_name parameter has not been set");
    }

    std::string tmpObjSize;
    helper::SetParameterValue("object_size", params, tmpObjSize);
    std::cout << "Parameter value (object): " << tmpObjSize << "\n";
    if (tmpObjSize.empty())
    {
        helper::Throw<std::invalid_argument>(
            "Toolkit", "transport::file::FileFlexNVMe", "SetParameters",
            "object_size parameter has not been set");
    }
    else
    {
        m_objectSize = static_cast<size_t>(
            helper::StringTo<size_t>(tmpObjSize, "Object size"));
    }
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

    if (flan_init(m_deviceUrl, nullptr, &pool_arg, obj_size,
                  &FileFlexNVMe::flanh))
    {
        // We have to reset the address to null because flan_init can change its
        // value even when it fails.
        FileFlexNVMe::flanh = nullptr;

        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileFlexNVMe", "Open",
            "failed to initialise flan in call to FlexNVMe open: " +
                ErrnoErrMsg());
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

size_t FileFlexNVMe::GetSize() {}

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

std::string FileFlexNVMe::CreateChunkName()
{
    if (m_baseName == "")
    {
        helper::Throw<std::range_error>(
            "Toolkit", "transport::file::FileFlexNVMe", "CreateChunkName",
            "Empty filenames are not supported");
    }

    std::string base = m_baseName;
    base += "#" + std::to_string(m_chunkWrites);
    m_chunkWrites += 1;

    return base;
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
