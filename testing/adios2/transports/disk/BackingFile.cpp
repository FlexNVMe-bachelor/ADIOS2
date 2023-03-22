#include "BackingFile.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::experimental::filesystem;

namespace Disk
{
BackingFile::BackingFile(size_t blockSize, size_t blockCount)
: m_blockSize(blockSize), m_blockCount(blockCount){};

BackingFile::BackingFile() = default;

BackingFile::~BackingFile() { Teardown(); }

void BackingFile::Create()
{
    CreateFile();
    CreateZeroedBuffer();
    WriteToDisk();
}

void BackingFile::Teardown()
{
    if (m_backingFile != nullptr)
    {
        // Close the stream
        std::fclose(m_backingFile);

        // Delete the file
        fs::remove(m_backingFilePath);
    }
}

void BackingFile::CreateFile()
{
    m_backingFilePath = fs::path(std::tmpnam(nullptr));
    m_backingFilePath.replace_filename("FlexNVMe_" +
                                       m_backingFilePath.filename().string());

    m_backingFile = fopen(m_backingFilePath.c_str(), "wb+");
    if (!m_backingFile)
    {
        throw std::runtime_error("Failed to open the backing file: " +
                                 m_backingFilePath.string());
    }
}

void BackingFile::CreateZeroedBuffer()
{
    size_t size = GetByteSize();
    m_buffer = new char[size];
    memset(m_buffer, 0, size);
}

void BackingFile::WriteToDisk()
{
    size_t size = GetByteSize();
    size_t written =
        fwrite(m_buffer, sizeof(char), GetByteSize(), m_backingFile);

    if (written != size)
    {
        throw std::runtime_error(
            "Did not write all objects, errorcode/written objects: " +
            std::to_string(written));
    }
}

auto BackingFile::GetPath() -> std::string
{
    return m_backingFilePath.string();
}

auto BackingFile::GetByteSize() -> size_t { return m_blockSize * m_blockCount; }
} // namespace Disk
