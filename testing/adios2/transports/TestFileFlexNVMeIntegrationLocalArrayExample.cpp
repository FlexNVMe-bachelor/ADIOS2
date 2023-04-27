#include <adios2.h>
#include <cstdlib>
#include <gtest/gtest.h>

#include "disk/DiskTestClass.h"
#include "util.h"

#include <algorithm> //std::for_each
#include <array>
#include <chrono>
#include <ios> //std::ios_base::failure
#include <iostream>
#include <memory>
#include <stdexcept> //std::invalid_argument std::exception
#include <string>
#include <thread>
#include <vector>

class FlexNVMeAdiosIntegrationTestSuite : public Disk::DiskTestClass
{
protected:
    FlexNVMeAdiosIntegrationTestSuite() : Disk::DiskTestClass(4096, 64, 32) {}
};

void localArrayWrite(const adios2::Params &flexnvmeParams)
{
    int rank = 0;
    const int NSTEPS = 5;

    // generate different random numbers on each process,
    // but always the same sequence at each run
    srand(rank * 32767);

    adios2::ADIOS adios;

    // v0 has the same size on every process at every step
    const size_t Nglobal = 6;
    std::vector<double> v0(Nglobal);

    // Application variables for output
    // random size per process, 5..10 each
    // v1 has different size on each process (but fixed over time)
    const unsigned int Nx = rand() % 6 + 5;
    // Local array, size is fixed over time on each process
    std::vector<double> v1(Nx);

    // random size per process, a different size at each step
    unsigned int Nelems;
    // Local array, size is changing over time on each process
    std::vector<double> v2;

    // Local array, size is changing over time on each process
    // Also, random number of processes will write it at each step
    std::vector<double> &v3 = v2;

    // Get io settings from the config file or
    // create one with default settings here
    adios2::IO io = adios.DeclareIO("Output");
    io.SetEngine("BP5");

    io.AddTransport("File", flexnvmeParams);

    /*
     * Define local array: type, name, local size
     * Global dimension and starting offset must be an empty vector
     * Here the size of the local array is the same on every process
     */
    adios2::Variable<double> varV0 =
        io.DefineVariable<double>("v0", {}, {}, {Nglobal});

    /*
     * v1 is similar to v0 but on every process the local size
     * is a different value
     */
    adios2::Variable<double> varV1 =
        io.DefineVariable<double>("v1", {}, {}, {Nx});

    /*
     * Define local array: type, name
     * Global dimension and starting offset must be an empty vector
     * but local size CANNOT be an empty vector.
     * We can use {adios2::UnknownDim} for this purpose or any number
     * actually since we will modify it before writing
     */
    adios2::Variable<double> varV2 =
        io.DefineVariable<double>("v2", {}, {}, {adios2::UnknownDim});

    /*
     * v3 is just like v2
     */
    adios2::Variable<double> varV3 =
        io.DefineVariable<double>("v3", {}, {}, {adios2::UnknownDim});

    // Open file. "w" means we overwrite any existing file on disk,
    // but Advance() will append steps to the same file.
    adios2::Engine writer = io.Open("localArray.bp", adios2::Mode::Write);

    for (int step = 0; step < NSTEPS; step++)
    {
        writer.BeginStep();

        // v0
        for (size_t i = 0; i < Nglobal; i++)
        {
            v0[i] = rank * 1.0 + step * 0.1;
        }
        writer.Put<double>(varV0, v0.data());

        // v1
        for (size_t i = 0; i < Nx; i++)
        {
            v1[i] = rank * 1.0 + step * 0.1;
        }
        writer.Put<double>(varV1, v1.data());

        // v2

        // random size per process per step, 5..10 each
        Nelems = rand() % 6 + 5;
        v2.reserve(Nelems);
        for (size_t i = 0; i < Nelems; i++)
        {
            v2[i] = rank * 1.0 + step * 0.1;
        }

        // Set the size of the array now because we did not know
        // the size at the time of definition
        varV2.SetSelection(adios2::Box<adios2::Dims>({}, {Nelems}));
        writer.Put<double>(varV2, v2.data());

        // v3

        // random chance who writes it
        unsigned int chance = rand() % 100;
        /*if (step == 2)
        {
            chance = 0;
        }*/
        bool doWrite = (chance > 60);
        if (doWrite)
        {
            varV3.SetSelection(adios2::Box<adios2::Dims>({}, {Nelems}));
            writer.Put<double>(varV3, v3.data());
        }

        writer.EndStep();
    }

    writer.Close();
}

std::vector<std::vector<double>> ReadVariable(const std::string &name,
                                              adios2::IO &io,
                                              adios2::Engine &reader,
                                              size_t step)
{
    adios2::Variable<double> variable = io.InquireVariable<double>(name);

    if (variable)
    {
        auto blocksInfo = reader.BlocksInfo(variable, step);

        // create a data vector for each block
        std::vector<std::vector<double>> dataSet;
        dataSet.resize(blocksInfo.size());

        // schedule a read operation for each block separately
        int i = 0;
        for (auto &info : blocksInfo)
        {
            variable.SetBlockSelection(info.BlockID);
            reader.Get<double>(variable, dataSet[i], adios2::Mode::Deferred);
            ++i;
        }

        // Read in all blocks at once now
        reader.PerformGets();
        // data vectors now are filled with data

        return dataSet;
    }

    return {};
}

using steps_dataset = std::vector<
    std::tuple<std::string, size_t, std::vector<std::vector<double>>>>;

steps_dataset localArrayRead(const adios2::Params &flexnvmeParams)
{
    adios2::ADIOS adios;

    /*** IO class object: settings and factory of Settings: Variables,
     * Parameters, Transports, and Execution: Engines
     * Inline uses single IO for write/read */
    adios2::IO io = adios.DeclareIO("Input");

    io.AddTransport("File", flexnvmeParams);

    adios2::Engine reader = io.Open("localArray.bp", adios2::Mode::Read);

    steps_dataset stepDatasets;

    while (true)
    {
        // Begin step
        adios2::StepStatus read_status =
            reader.BeginStep(adios2::StepMode::Read, 10.0f);
        if (read_status == adios2::StepStatus::NotReady)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        else if (read_status != adios2::StepStatus::OK)
        {
            break;
        }

        size_t step = reader.CurrentStep();

        std::vector<std::string> vars = {"v0", "v1", "v2", "v3"};
        for (auto &var : vars)
        {
            auto dataSet = ReadVariable(var, io, reader, step);
            stepDatasets.push_back(std::make_tuple(var, step, dataSet));
        }

        reader.EndStep();
    }

    reader.Close();

    return stepDatasets;
}

TEST_F(FlexNVMeAdiosIntegrationTestSuite, CanRunLocalArrayExampleTest)
{
    adios2::Params flexnvmeParams = GetParams();
    localArrayWrite(flexnvmeParams);
    steps_dataset stepDatasets = localArrayRead(flexnvmeParams);

    steps_dataset expected{{"v0", 0, {{0, 0, 0, 0, 0, 0}}},
                           {"v1", 0, {{0, 0, 0, 0, 0, 0}}},
                           {"v2", 0, {{0, 0, 0, 0, 0, 0, 0, 0, 0}}},
                           {"v3", 0, {{0, 0, 0, 0, 0, 0, 0, 0, 0}}},
                           {"v0", 1, {{0.1, 0.1, 0.1, 0.1, 0.1, 0.1}}},
                           {"v1", 1, {{0.1, 0.1, 0.1, 0.1, 0.1, 0.1}}},
                           {"v2", 1, {{0.1, 0.1, 0.1, 0.1, 0.1, 0.1}}},
                           {"v3", 1, {{0.1, 0.1, 0.1, 0.1, 0.1, 0.1}}},
                           {"v0", 2, {{0.2, 0.2, 0.2, 0.2, 0.2, 0.2}}},
                           {"v1", 2, {{0.2, 0.2, 0.2, 0.2, 0.2, 0.2}}},
                           {"v2", 2, {{0.2, 0.2, 0.2, 0.2, 0.2, 0.2}}},
                           {"v3", 2, {{0.2, 0.2, 0.2, 0.2, 0.2, 0.2}}},
                           {"v0", 3, {{0.3, 0.3, 0.3, 0.3, 0.3, 0.3}}},
                           {"v1", 3, {{0.3, 0.3, 0.3, 0.3, 0.3, 0.3}}},
                           {"v2", 3, {{0.3, 0.3, 0.3, 0.3, 0.3}}},
                           {"v3", 3, {}},
                           {"v0", 4, {{0.4, 0.4, 0.4, 0.4, 0.4, 0.4}}},
                           {"v1", 4, {{0.4, 0.4, 0.4, 0.4, 0.4, 0.4}}},
                           {"v2", 4, {{0.4, 0.4, 0.4, 0.4, 0.4, 0.4}}},
                           {"v3", 4, {{0.4, 0.4, 0.4, 0.4, 0.4, 0.4}}}};

    // Must manually assert to avoid floating point errors causing test failure
    ASSERT_EQ(stepDatasets.size(), expected.size());
    for (int i = 0; i < stepDatasets.size(); i++)
    {
        auto step = stepDatasets[i], expectedStep = expected[i];
        ASSERT_EQ(std::get<0>(step), std::get<0>(expectedStep));
        ASSERT_EQ(std::get<1>(step), std::get<1>(expectedStep));

        ASSERT_EQ(std::get<2>(step).size(), std::get<2>(expectedStep).size());

        for (int j = 0; j < std::get<2>(step).size(); j++)
        {
            auto dataset = std::get<2>(step)[j],
                 expectedDataset = std::get<2>(expectedStep)[j];

            ASSERT_EQ(dataset.size(), expectedDataset.size());

            for (int k = 0; k < dataset.size(); k++)
            {
                auto datum = dataset[k], expectedDatum = expectedDataset[k];
                ASSERT_DOUBLE_EQ(datum, expectedDatum);
            }
        }
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));
    return RUN_ALL_TESTS();
}
