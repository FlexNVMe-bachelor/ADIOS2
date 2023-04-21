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

    size_t size = transport.GetSize();

    ASSERT_EQ(size, dataSize);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));
    return RUN_ALL_TESTS();
}
