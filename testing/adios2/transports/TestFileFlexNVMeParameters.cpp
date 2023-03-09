#include "adios2/cxx11/ADIOS.h"
#include <gtest/gtest.h>

TEST(ParameterSuite, DefaultBehaviourTest)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("TestIO");

    io.AddTransport("File", {{"Library", "flexnvme"},
                             {"device_url", "/tmp/loop"},
                             {"object_size", "128"},
                             {"pool_name", "mypool"}});

    adios2::Engine engine = io.Open("test.bp", adios2::Mode::Write);
}

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);

    result = RUN_ALL_TESTS();

    return result;
}
