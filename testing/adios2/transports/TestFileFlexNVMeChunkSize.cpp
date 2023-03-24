#include <gtest/gtest.h>
#include <stdexcept>

#include "adios2/helper/adiosCommDummy.h"
#include "adios2/toolkit/transport/file/FileFlexNVMe.h"

#include "disk/DiskTestClass.h"

class ChunkSizeTestSuite : public Disk::DiskTestClass
{
protected:
    ChunkSizeTestSuite() : Disk::DiskTestClass(4096, 64, 32) {}
};

TEST_F(ChunkSizeTestSuite, DefaultBehaviourTest)
{
    adios2::transport::FileFlexNVMe e(adios2::helper::CommDummy());
    e.SetParameters(GetParams());
    e.Open("helloworld", adios2::Mode::Write);

    const size_t size = m_blockSize;
    char buf[size];
    ASSERT_NO_THROW(e.Write(buf, size, 0));
    ASSERT_NO_THROW(e.Write(buf, size, 0));
}

TEST_F(ChunkSizeTestSuite, BiggerSizeThrowsTest)
{
    adios2::transport::FileFlexNVMe e(adios2::helper::CommDummy());
    e.SetParameters(GetParams());
    e.Open("helloworld", adios2::Mode::Write);

    const size_t size = m_blockSize;
    char buf[size];
    ASSERT_NO_THROW(e.Write(buf, size, 0));
    ASSERT_THROW(e.Write(buf, size + 1, 0), std::invalid_argument);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
