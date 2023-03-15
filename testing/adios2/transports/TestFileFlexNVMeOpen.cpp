#include <gtest/gtest.h>

#include "adios2/helper/adiosCommDummy.h"
#include "adios2/toolkit/transport/file/FileFlexNVMe.h"
#include "disk/loopback.h"
#include <flexalloc_mm.h>

class OpenTestSuite : public ::testing::Test
{
protected:
    Disk::LoopbackDevice device;

    OpenTestSuite()
    {
        Disk::LoopbackDevice dev("/tmp/testloop.img");
        device = dev;
        device.Create();
    }
};

TEST_F(OpenTestSuite, CanOpenTest)
{
    adios2::transport::FileFlexNVMe e(adios2::helper::CommDummy());
    e.deviceUrl = const_cast<char *>(device.GetDeviceUrl().c_str());

    e.Open("helloworld", adios2::Mode::Write);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
