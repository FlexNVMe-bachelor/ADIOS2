#include <adios2.h>
#include <cstdlib>
#include <gtest/gtest.h>

#include "disk/DiskTestClass.h"
#include "util.h"

#include <adios2.h>
#include <iostream>
#include <stdexcept>

class FlexNVMeAdiosIntegrationHelloWorldTestSuite : public Disk::DiskTestClass
{
protected:
    FlexNVMeAdiosIntegrationHelloWorldTestSuite()
    : Disk::DiskTestClass(4096, 64, 32)
    {
    }
};

void writer(adios2::ADIOS &adios, const std::string &greeting,
            adios2::Params &flexnvmeParams)
{
    adios2::IO io = adios.DeclareIO("hello-world-writer");

    io.AddTransport("File", flexnvmeParams);

    adios2::Variable<std::string> varGreeting =
        io.DefineVariable<std::string>("Greeting");

    adios2::Engine writer = io.Open("hello-world-cpp.bp", adios2::Mode::Write);
    writer.Put(varGreeting, greeting);
    writer.Close();
}

std::string reader(adios2::ADIOS &adios, adios2::Params &flexnvmeParams)
{
    adios2::IO io = adios.DeclareIO("hello-world-reader");

    io.AddTransport("File", flexnvmeParams);

    adios2::Engine reader = io.Open("hello-world-cpp.bp", adios2::Mode::Read);
    reader.BeginStep();
    adios2::Variable<std::string> varGreeting =
        io.InquireVariable<std::string>("Greeting");
    std::string greeting;
    reader.Get(varGreeting, greeting);
    reader.EndStep();
    reader.Close();
    return greeting;
}

TEST_F(FlexNVMeAdiosIntegrationHelloWorldTestSuite, CanRunHelloWorldExampleTest)
{
    adios2::Params flexnvmeParams = GetParams();
    adios2::ADIOS adios;

    const std::string greeting = "Hello World from ADIOS2";
    writer(adios, greeting, flexnvmeParams);
    const std::string message = reader(adios, flexnvmeParams);

    ASSERT_EQ(greeting, message);
}

TEST_F(FlexNVMeAdiosIntegrationHelloWorldTestSuite,
       CanRunLargerHelloWorldExampleTest)
{
    adios2::Params flexnvmeParams = GetParams();
    adios2::ADIOS adios;

    std::string greeting = "Let's learn how to count:";

    const size_t APPROX_MAX_LEN = 12 * m_blockSize;

    size_t i = 0;
    while (greeting.length() < APPROX_MAX_LEN)
    {
        greeting += " " + std::to_string(i++);
    }

    writer(adios, greeting, flexnvmeParams);
    const std::string message = reader(adios, flexnvmeParams);

    ASSERT_EQ(greeting, message);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));
    return RUN_ALL_TESTS();
}
