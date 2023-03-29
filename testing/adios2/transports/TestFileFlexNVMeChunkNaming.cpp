#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>

#include "adios2/common/ADIOSTypes.h"
#include "adios2/helper/adiosCommDummy.h"
#include "adios2/toolkit/transport/file/FileFlexNVMe.h"

#include "disk/DiskTestClass.h"

using ParamType = std::tuple<std::string, size_t, std::string>;

class ChunkNameSuite : public Disk::DiskTestClassWithParams<ParamType>
{
protected:
    ChunkNameSuite() : Disk::DiskTestClassWithParams<ParamType>(4096, 64, 32) {}
};

TEST_P(ChunkNameSuite, CreateChunkNameTest)
{
    const std::string &baseName = std::get<0>(GetParam());
    const size_t &chunkNum = std::get<1>(GetParam());
    const std::string &expected = std::get<2>(GetParam());

    adios2::transport::FileFlexNVMe e(adios2::helper::CommDummy());
    e.SetParameters(GetParams());
    e.Open(baseName, adios2::Mode::Write);

    ASSERT_EQ(e.GenerateChunkName(chunkNum), expected);
}

INSTANTIATE_TEST_SUITE_P(
    NameValues, ChunkNameSuite,
    ::testing::Values(std::make_tuple("basename", 0, "basename#0"),
                      std::make_tuple("basename.0", 0, "basename.0#0"),
                      std::make_tuple("hello/world/", 1, "hello_world_#1"),
                      std::make_tuple("hello/world#", 1, "hello_world#1#1")));

TEST_F(ChunkNameSuite, EmptyNameThrowsTest)
{
    adios2::transport::FileFlexNVMe e(adios2::helper::CommDummy());
    e.SetParameters(GetParams());
    e.Open("", adios2::Mode::Write);

    ASSERT_THROW(e.GenerateChunkName(0), std::range_error);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
