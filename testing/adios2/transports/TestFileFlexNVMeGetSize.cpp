#include <cstdlib>
#include <gtest/gtest.h>

#include "adios2/helper/adiosCommDummy.h"
#include "adios2/toolkit/transport/file/FileFlexNVMe.h"

#include "disk/DiskTestClass.h"
#include "util.h"

#include <memory>
#include <string>

class GetSizeTestSuite : public Disk::DiskTestClass
{
protected:
    GetSizeTestSuite() : Disk::DiskTestClass(4096, 64, 32) {}
};

TEST_F(GetSizeTestSuite, CanGetSizeOfSingleChunkTest)
{
    adios2::transport::FileFlexNVMe transport(adios2::helper::CommDummy());
    transport.SetParameters(GetParams());

    transport.Open("helloworld", adios2::Mode::Write);

    Rng rng;
    size_t dataSize = rng.RandRange(1, m_blockSize);

    std::string data = rng.RandString(dataSize - 1, 'a', 'z');

    transport.Write(data.c_str(), dataSize, 0);

    ASSERT_EQ(dataSize, transport.GetSize());
}

TEST_F(GetSizeTestSuite, CanGetSizeOfMultipleChunksTest)
{
    adios2::transport::FileFlexNVMe transport(adios2::helper::CommDummy());
    transport.SetParameters(GetParams());

    transport.Open("helloworld", adios2::Mode::Write);

    Rng rng;
    const size_t MIN_NUM_CHUNKS = 1, MAX_NUM_CHUNKS = 32;
    size_t dataSize = rng.RandRange(MIN_NUM_CHUNKS * m_blockSize,
                                    MAX_NUM_CHUNKS * m_blockSize);

    std::string data = rng.RandString(dataSize - 1);

    transport.Write(data.c_str(), dataSize, 0);

    ASSERT_EQ(dataSize, transport.GetSize());
}

TEST_F(GetSizeTestSuite, CannotGetSizeOfNonExistentObjectTest)
{
    adios2::transport::FileFlexNVMe transport(adios2::helper::CommDummy());
    transport.SetParameters(GetParams());

    transport.Open("helloworld", adios2::Mode::Read);

    ASSERT_THROW(transport.GetSize(), std::invalid_argument);
}

TEST_F(GetSizeTestSuite, CanGetSizeOfFullChunksTest)
{
    adios2::transport::FileFlexNVMe transport(adios2::helper::CommDummy());
    transport.SetParameters(GetParams());

    transport.Open("helloworld", adios2::Mode::Write);

    Rng rng;
    const size_t MIN_NUM_CHUNKS = 1, MAX_NUM_CHUNKS = 16;
    size_t numChunks = rng.RandRange(MIN_NUM_CHUNKS, MAX_NUM_CHUNKS);
    size_t dataSize = numChunks * m_blockSize;

    std::string data = rng.RandString(dataSize - 1);

    transport.Write(data.c_str(), dataSize, 0);

    ASSERT_EQ(dataSize, transport.GetSize());
}

TEST_F(GetSizeTestSuite, CanGetSizeOfSingleByteChunkTest)
{
    adios2::transport::FileFlexNVMe transport(adios2::helper::CommDummy());
    transport.SetParameters(GetParams());

    transport.Open("helloworld", adios2::Mode::Write);

    Rng rng;
    const size_t MIN_NUM_CHUNKS = 1, MAX_NUM_CHUNKS = 16;
    size_t numChunks = rng.RandRange(MIN_NUM_CHUNKS, MAX_NUM_CHUNKS);
    size_t dataSize = numChunks * m_blockSize + 1;

    std::string data = rng.RandString(dataSize - 1);

    transport.Write(data.c_str(), dataSize, 0);

    ASSERT_EQ(dataSize, transport.GetSize());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));
    return RUN_ALL_TESTS();
}
