#ifndef BACKINGFILE_H
#define BACKINGFILE_H

#include <cstdio>
#include <experimental/filesystem>
#include <string>

#define BACKING_FILE_DEFAULT_BLOCK_SIZE 4096
#define BACKING_FILE_DEFAULT_BLOCK_COUNT 64

namespace Disk
{
class BackingFile
{
public:
    explicit BackingFile();
    explicit BackingFile(size_t blockSize, size_t blockCount);
    ~BackingFile();

    void Create();
    auto GetPath() -> std::string;

private:
    std::experimental::filesystem::path m_backingFilePath = "";
    std::FILE *m_backingFile = nullptr;

    size_t m_blockSize = BACKING_FILE_DEFAULT_BLOCK_SIZE;
    size_t m_blockCount = BACKING_FILE_DEFAULT_BLOCK_COUNT;
    char *m_buffer = nullptr;

    void CreateFile();
    void CreateZeroedBuffer();
    void WriteToDisk();

    auto GetByteSize() -> size_t;
};
} // namespace Disk

#endif
