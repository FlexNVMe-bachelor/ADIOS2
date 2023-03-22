#include <gtest/gtest.h>
#include <string>

#include "adios2/cxx11/ADIOS.h"
#include "disk/BackingFile.h"
#include "disk/Loopback.h"
#include "disk/mkfs.h"

class OpenTestSuite : public ::testing::Test
{
protected:
    Disk::LoopbackDevice device;
    Disk::BackingFile backingFile;
    size_t objSize = 64;

    OpenTestSuite()
    {
        Disk::BackingFile bf(4096, objSize);
        backingFile = bf;
        backingFile.Create();

        Disk::LoopbackDevice dev(backingFile.GetPath());
        device = dev;
        device.Create();

        mkfs(device.GetDeviceUrl(), 32);
    }
};

TEST_F(OpenTestSuite, DefaultBehaviourTest)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("TestIO");

    io.AddTransport("File", {{"Library", "flexnvme"},
                             {"device_url", device.GetDeviceUrl()},
                             {"object_size", std::to_string(objSize)},
                             {"pool_name", "mypool"}});

    adios2::Engine engine = io.Open("test.bp", adios2::Mode::Write);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
