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
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <string>

#include <flan.h>
#include <utility>

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

void FileFlexNVMe::SetParameters(const Params &params)
{
    helper::SetParameterValue("device_url", params, m_deviceUrl);
    if (m_deviceUrl.empty())
    {
        helper::Throw<std::invalid_argument>(
            "Toolkit", "transport::file::FileFlexNVMe", "SetParameters",
            "device_url parameter has not been set");
    }

    helper::SetParameterValue("pool_name", params, m_poolName);
    if (m_poolName.empty())
    {
        helper::Throw<std::invalid_argument>(
            "Toolkit", "transport::file::FileFlexNVMe", "SetParameters",
            "pool_name parameter has not been set");
    }

    std::string tmpObjSize;
    helper::SetParameterValue("object_size", params, tmpObjSize);
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

// TODO(adbo):
//  - Async open/close?
//  - Remove printf/cout
void FileFlexNVMe::Open(const std::string &name, const Mode openMode,
                        const bool /*async*/, const bool /*directio*/)
{
    m_Name = name;
    m_OpenMode = openMode;
    m_baseName = NormalisedObjectName(const_cast<std::string &>(name));
    m_Cursor = 0;

    switch (m_OpenMode)
    {
    case Mode::Write:
    case Mode::Read:
        break;

    // TODO(adbo): more open modes?
    default:
        helper::Throw<std::invalid_argument>(
            "Toolkit", "transport::file::FileFlexNVMe", "Open",
            "unknown open mode for file " + m_Name +
                " in call to FlexNVMe open");
    }

    ProfilerStart("open");

    InitFlan(m_poolName);
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

    uint64_t obj_size = m_objectSize;

    if (flan_init(const_cast<char *>(m_deviceUrl.c_str()), nullptr, &pool_arg,
                  obj_size, &FileFlexNVMe::flanh))
    {
        // We have to reset the address to null because flan_init can change its
        // value even when it fails.
        FileFlexNVMe::flanh = nullptr;

        std::string err =
            "failed to initialise flan in call to FlexNVMe open: " +
            ErrnoErrMsg();

        if (errno == 22)
        {
            helper::Throw<std::runtime_error>(
                "Toolkit", "transport::file::FileFlexNVMe", "Open",
                err + ". You may need to mkfs.flexalloc the storage device.");
        }

        helper::Throw<std::runtime_error>(
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

void FileFlexNVMe::Write(const char *buffer, size_t size, size_t start)
{
    if (start == MaxSizeT)
    {
        start = m_Cursor;
    }
    else
    {
        m_Cursor = start;
    }

    if (size == 0)
    {
        return;
    }

    ChunkLocation startLoc = CalculateChunkLocation(start);
    ChunkLocation endLoc = CalculateChunkLocation(start + size);

    size_t curChunk = startLoc.chunkNum;
    const char *writePointer = buffer;
    while (curChunk <= endLoc.chunkNum)
    {
        size_t subSize = m_objectSize;
        size_t subOffset = 0;

        // In the first chunk, we can start at any point in the chunk
        if (curChunk == startLoc.chunkNum)
        {
            subSize -= startLoc.chunkOffset;
            subOffset = startLoc.chunkOffset;
        }

        // In the last chunk (which can possibly the same as the first chunk),
        // we might have to not write until the end of the chunk
        if (curChunk == endLoc.chunkNum)
        {
            subSize = endLoc.chunkOffset - subOffset;
        }

        if (subSize == 0)
        {
            break;
        }

        std::string objectName = GenerateChunkName(curChunk);

        uint64_t objectHandle = OpenFlanObject(
            objectName, FLAN_OPEN_FLAG_CREATE | FLAN_OPEN_FLAG_WRITE);

        if (flan_object_write(objectHandle, const_cast<char *>(writePointer),
                              subOffset, subSize, FileFlexNVMe::flanh))
        {
            helper::Throw<std::runtime_error>(
                "Toolkit", "transport::file::FileFlexNVMe", "Write",
                "Failed to write chunk " + objectName);
        }

        CloseFlanObject(objectHandle);

        curChunk++;
        writePointer += subSize;
    }

    m_Cursor += size;
}

#ifdef REALLY_WANT_WRITEV
void FileFlexNVMe::WriteV(const core::iovec *iov, const int iovcnt,
                          size_t start)
{
}
#endif

void FileFlexNVMe::Read(char *buffer, size_t size, size_t start)
{
    if (start == MaxSizeT)
    {
        start = m_Cursor;
    }
    else
    {
        m_Cursor = start;
    }

    if (size == 0)
    {
        return;
    }

    ChunkLocation startLoc = CalculateChunkLocation(start);
    ChunkLocation endLoc = CalculateChunkLocation(start + size);

    size_t curChunk = startLoc.chunkNum;
    char *readPointer = buffer;
    while (curChunk <= endLoc.chunkNum)
    {
        size_t subSize = m_objectSize;
        size_t subOffset = 0;

        // In the first chunk, we can start at any point in the chunk
        if (curChunk == startLoc.chunkNum)
        {
            subSize -= startLoc.chunkOffset;
            subOffset = startLoc.chunkOffset;
        }

        // In the last chunk (which can possibly the same as the first chunk),
        // we might have to not read until the end of the chunk
        if (curChunk == endLoc.chunkNum)
        {
            subSize = endLoc.chunkOffset - subOffset;
        }

        if (subSize == 0)
        {
            break;
        }

        std::string objectName = GenerateChunkName(curChunk);

        // This will throw if the chunk doesn't exist
        uint64_t objectHandle = OpenFlanObject(objectName);
        ssize_t numBytesRead = flan_object_read(
            objectHandle, readPointer, subOffset, subSize, FileFlexNVMe::flanh);
        CloseFlanObject(objectHandle);

        if (numBytesRead < subSize)
        {
            helper::Throw<std::range_error>(
                "Toolkit", "transport::file::FileFlexNVMe", "Read",
                "Failed to read '" + m_Name +
                    "' because more bytes were requested than were stored");
        }

        curChunk++;
        readPointer += subSize;
    }

    m_Cursor += size;
}

size_t FileFlexNVMe::GetSize()
{
    if (FileFlexNVMe::flanh == nullptr)
    {
        helper::Throw<std::logic_error>(
            "Toolkit", "transport::file::FileFlexNVMe", "GetSize",
            "GetSize called before Open");
    }

    struct flan_oinfo *objectInfo = nullptr;
    int chunkNum = 0;
    size_t totalSize = 0;

    // TODO(adbo): extract to function like GetChunkInfo(chunkName)
    while ((objectInfo =
                flan_find_oinfo(FileFlexNVMe::flanh,
                                GenerateChunkName(chunkNum).c_str(), nullptr)))
    {
        totalSize += objectInfo->size;
        chunkNum++;
    }

    if (chunkNum == 0)
    {
        helper::Throw<std::invalid_argument>(
            "Toolkit", "transport::file::FileFlexNVMe", "GetSize",
            "Object '" + m_Name + "' not found");
    }

    return totalSize;
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
    m_Cursor = 0;
}

void FileFlexNVMe::Delete() {}

void FileFlexNVMe::SeekToEnd() { m_Cursor = GetSize(); }

void FileFlexNVMe::SeekToBegin() { m_Cursor = 0; }

void FileFlexNVMe::Seek(const size_t start)
{
    if (start != MaxSizeT)
    {
        m_Cursor = start;
    }
    else
    {
        SeekToEnd();
    }
}

void FileFlexNVMe::Truncate(const size_t length) {}

void FileFlexNVMe::MkDir(const std::string &fileName) {}

std::string FileFlexNVMe::NormalisedObjectName(std::string &input)
{
    std::string normalised = input;
    std::replace(normalised.begin(), normalised.end(), '/', '_');
    return normalised;
}

std::string FileFlexNVMe::GenerateChunkName(size_t chunkNum)
{
    if (m_baseName == "")
    {
        helper::Throw<std::range_error>(
            "Toolkit", "transport::file::FileFlexNVMe", "GenerateChunkName",
            "Empty filenames are not supported");
    }

    std::string base = m_baseName;
    base += "#" + std::to_string(chunkNum);
    return base;
}

inline auto FileFlexNVMe::CalculateChunkLocation(size_t offset) -> ChunkLocation
{
    return {.chunkNum = offset / m_objectSize,
            .chunkOffset = offset % m_objectSize};
}

auto FileFlexNVMe::OpenFlanObject(std::string &objectName, int flags)
    -> uint64_t
{
    uint64_t objectHandle = 0;
    if (flan_object_open(objectName.c_str(), FileFlexNVMe::flanh, &objectHandle,
                         flags))
    {
        helper::Throw<std::invalid_argument>(
            "Toolkit", "transport::file::FileFlexNVMe", "OpenChunk",
            "Failed to open object " + objectName);
    }
    return objectHandle;
}

void FileFlexNVMe::CloseFlanObject(uint64_t objectHandle)
{
    if (flan_object_close(objectHandle, FileFlexNVMe::flanh))
    {
        helper::Throw<std::runtime_error>(
            "Toolkit", "transport::file::FileFlexNVMe", "CloseChunk",
            "Failed to close chunk object");
    }
}

} // end namespace transport
} // end namespace adios2
