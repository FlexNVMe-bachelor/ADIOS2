#include <gtest/gtest.h>
#include <string>

#include "adios2/cxx11/ADIOS.h"

#include "disk/DiskTestClass.h"

class ParameterTestSuite : public Disk::DiskTestClass
{
protected:
    ParameterTestSuite() : Disk::DiskTestClass(4096, 64, 32) {}
};

TEST_F(ParameterTestSuite, DefaultBehaviourTest)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("TestIO");

    // This does creates the same params as GetParams()
    io.AddTransport("File", {{"Library", "flexnvme"},
                             {"device_url", device.GetDeviceUrl()},
                             {"object_size", std::to_string(m_blockSize)},
                             {"pool_name", "mypool"}});

    adios2::Engine engine = io.Open("test.bp", adios2::Mode::Write);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
