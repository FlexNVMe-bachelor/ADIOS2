#include <cstdlib>
#include <gtest/gtest.h>

#include "adios2/helper/adiosCommDummy.h"
#include "adios2/toolkit/transport/file/FileFlexNVMe.h"

#include "disk/DiskTestClass.h"

#include <memory>
#include <random>
#include <string>

class ReadWriteTestSuite : public Disk::DiskTestClass
{
protected:
    ReadWriteTestSuite() : Disk::DiskTestClass(4096, 64, 32) {}
};

class Rng
{
private:
    std::random_device rd;

public:
    Rng()
    {
        // From
        // https://stackoverflow.com/questions/13445688/how-to-generate-a-random-number-in-c
        std::mt19937::result_type seed =
            rd() ^ ((std::mt19937::result_type)
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count() +
                    (std::mt19937::result_type)
                        std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::high_resolution_clock::now()
                                .time_since_epoch())
                            .count());

        std::mt19937 gen(seed);
    }

    auto RandRange(size_t min, size_t max) -> size_t
    {
        std::uniform_int_distribution<unsigned> distrib(min, max);
        return distrib(rd);
    }

    auto RandString(size_t length) -> std::string
    {
        const char ASCII_RANGE_START = 33, ASCII_RANGE_END = 122;
        return RandString(length, ASCII_RANGE_START, ASCII_RANGE_END);
    }

    auto RandString(size_t length, char asciiStart, char asciiEnd)
        -> std::string
    {
        std::string data = "";

        for (int i = 0; i < length; i++)
        {
            char randomChar =
                static_cast<char>(RandRange(asciiStart, asciiEnd));
            data += randomChar;
        }

        return data;
    }
};

TEST_F(ReadWriteTestSuite, CanWriteAndReadSingleChunkTest)
{
    adios2::transport::FileFlexNVMe transport(adios2::helper::CommDummy());
    transport.SetParameters(GetParams());

    transport.Open("helloworld", adios2::Mode::Write);

    Rng rng;
    size_t bufferSize = rng.RandRange(1, m_blockSize);

    std::string data = rng.RandString(
        bufferSize - 1, 'a', 'z'); // -1 to have space for null terminator

    transport.Write(data.c_str(), bufferSize, 0);

    char *readBuffer = (char *)malloc(bufferSize);
    transport.Read(readBuffer, bufferSize, 0);
    std::string readData(readBuffer);
    free(readBuffer);

    ASSERT_EQ(data, readData);
}

TEST_F(ReadWriteTestSuite, CanWriteAndReadMultipleChunksTest)
{
    adios2::transport::FileFlexNVMe transport(adios2::helper::CommDummy());
    transport.SetParameters(GetParams());

    transport.Open("helloworld", adios2::Mode::Write);

    Rng rng;
    const size_t MIN_NUM_CHUNKS = 2, MAX_NUM_CHUNKS = 32;
    size_t bufferSize = rng.RandRange(MIN_NUM_CHUNKS * m_blockSize,
                                      MAX_NUM_CHUNKS * m_blockSize);

    std::string data = rng.RandString(bufferSize - 1);

    transport.Write(data.c_str(), bufferSize, 0);

    char *readBuffer = (char *)malloc(bufferSize);
    transport.Read(readBuffer, bufferSize, 0);
    std::string readData(readBuffer);
    free(readBuffer);

    ASSERT_EQ(data, readData);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));
    return RUN_ALL_TESTS();
}
