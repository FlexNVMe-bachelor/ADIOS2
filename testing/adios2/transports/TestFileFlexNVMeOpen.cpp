#include <gtest/gtest.h>

#include "adios2/helper/adiosCommDummy.h"
#include "adios2/toolkit/transport/file/FileFlexNVMe.h"

#include "disk/DiskTestClass.h"

class OpenTestSuite : public Disk::DiskTestClass
{
protected:
    OpenTestSuite() : Disk::DiskTestClass(4096, 64, 32) {}
};

TEST_F(OpenTestSuite, CanOpenTest)
{
    adios2::transport::FileFlexNVMe e(adios2::helper::CommDummy());
    e.SetParameters(GetParams());

    e.Open("helloworld", adios2::Mode::Write);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
