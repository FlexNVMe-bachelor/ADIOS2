#include "adios2/common/ADIOSTypes.h"
#include "adios2/helper/adiosCommDummy.h"
#include "adios2/toolkit/transport/file/FileFlexNVMe.h"
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>

class ChunkNameSuite
: public ::testing::TestWithParam<
      std::tuple<std::string, std::string, std::string, std::string>>
{
};

TEST_P(ChunkNameSuite, CreateChunkNameTest)
{
    const std::string &baseName = std::get<0>(GetParam());
    const std::string &first = std::get<1>(GetParam());
    const std::string &second = std::get<2>(GetParam());
    const std::string &third = std::get<3>(GetParam());

    adios2::transport::FileFlexNVMe e(adios2::helper::CommDummy());
    e.Open(baseName, adios2::Mode::Undefined);
    ASSERT_EQ(e.IncrementChunkName(), first);
    ASSERT_EQ(e.IncrementChunkName(), second);
    ASSERT_EQ(e.IncrementChunkName(), third);
}

INSTANTIATE_TEST_SUITE_P(
    NameValues, ChunkNameSuite,
    ::testing::Values(std::make_tuple("basename", "basename#0", "basename#1",
                                      "basename#2"),
                      std::make_tuple("basename.0", "basename.0#0",
                                      "basename.0#1", "basename.0#2"),
                      std::make_tuple("helloworld.1.1", "helloworld.1.1#0",
                                      "helloworld.1.1#1", "helloworld.1.1#2"),
                      std::make_tuple("helloworld#1", "helloworld#1#0",
                                      "helloworld#1#1", "helloworld#1#2")));

TEST(ChunkNameSuite, EmptyNameThrowsTest)
{
    adios2::transport::FileFlexNVMe e(adios2::helper::CommDummy());
    e.Open("", adios2::Mode::Undefined);

    ASSERT_THROW(e.IncrementChunkName(), std::range_error);
}

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);

    result = RUN_ALL_TESTS();

    return result;
}
