#include <gtest/gtest.h>

#include "adios2/helper/adiosCommDummy.h"
#include "adios2/toolkit/transport/file/FileFlexNVMe.h"
#include "disk/BackingFile.h"
#include "disk/Loopback.h"
#include "disk/mkfs.h"
#include <flexalloc_mm.h>
#include <iostream>

class OpenTestSuite : public ::testing::Test
{
protected:
    Disk::LoopbackDevice device;
    Disk::BackingFile backingFile;

    OpenTestSuite()
    {
        Disk::BackingFile bf(4096, 64);
        backingFile = bf;
        backingFile.Create();

        Disk::LoopbackDevice dev(backingFile.GetPath());
        device = dev;
        device.Create();

        mkfs(device.GetDeviceUrl(), 32);
    }
};

TEST_F(OpenTestSuite, CanOpenTest)
{
    adios2::transport::FileFlexNVMe e(adios2::helper::CommDummy());
    char *deviceUrl = const_cast<char *>(device.GetDeviceUrl().c_str());
    // e.SetDevice(deviceUrl);

    // std::cout << "flexnvme deviceUrl: " << e.deviceUrl << "\n";

    // e.Open(deviceUrl, adios2::Mode::Write);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
