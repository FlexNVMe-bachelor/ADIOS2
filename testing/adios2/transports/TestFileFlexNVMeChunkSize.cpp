#include "adios2/helper/adiosCommDummy.h"
#include "adios2/toolkit/transport/file/FileFlexNVMe.h"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(ChunkSizingSuite, DefaultBehaviourTest)
{
    adios2::transport::FileFlexNVMe e(adios2::helper::CommDummy());
    e.Open("helloworld", adios2::Mode::Undefined);

    const size_t size = 10;
    char buf[size];
    ASSERT_NO_THROW(e.Write(buf, size, 0));
    ASSERT_NO_THROW(e.Write(buf, size, 0));
}

TEST(ChunkSizingSuite, SmallerSizeWarnsTest)
{
    adios2::transport::FileFlexNVMe e(adios2::helper::CommDummy());
    e.Open("helloworld", adios2::Mode::Undefined);

    const size_t size = 10;
    char buf[size];
    ASSERT_NO_THROW(e.Write(buf, 10, 0));
    ASSERT_NO_THROW(e.Write(buf, 5, 0));
}

TEST(ChunkSizingSuite, BiggerSizeThrowsTest)
{
    adios2::transport::FileFlexNVMe e(adios2::helper::CommDummy());
    e.Open("helloworld", adios2::Mode::Undefined);

    const size_t size = 10;
    char buf[size];
    ASSERT_NO_THROW(e.Write(buf, size, 0));
    ASSERT_THROW(e.Write(buf, 15, 0), std::invalid_argument);
}

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);

    result = RUN_ALL_TESTS();

    return result;
}
