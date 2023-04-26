#include <cstdlib>
#include <gtest/gtest.h>

#include "adios2/common/ADIOSTypes.h"
#include "adios2/helper/adiosCommDummy.h"
#include "adios2/toolkit/transport/Transport.h"
#include "adios2/toolkit/transport/file/FileFlexNVMe.h"
#include "adios2/toolkit/transport/file/FilePOSIX.h"

#include "disk/DiskTestClass.h"
#include "util.h"

#include <memory>
#include <random>
#include <string>
#include <utility>

class FileFlexNVMeFilePOSIXMatchTestSuite : public Disk::DiskTestClass
{
protected:
    FileFlexNVMeFilePOSIXMatchTestSuite() : Disk::DiskTestClass(4096, 64, 32) {}
};

TEST_F(FileFlexNVMeFilePOSIXMatchTestSuite, SingleWriteAndReadYieldsTheSame)
{
    Rng rng;

    adios2::transport::FilePOSIX posix(adios2::helper::CommDummy());
    adios2::transport::FileFlexNVMe flexnvme(adios2::helper::CommDummy());
    flexnvme.SetParameters(GetParams());

    std::string name = rng.RandString(24, 'a', 'z');

    size_t bufferSize = rng.RandRange(1, m_blockSize);
    std::string writeData = rng.RandString(bufferSize - 1);

    auto performTestOnTransport = [&](adios2::Transport &transport) {
        transport.Open(name, adios2::Mode::Write);
        transport.Write(writeData.c_str(), bufferSize, adios2::MaxSizeT);
        transport.Close();

        char *readBuffer = (char *)malloc(bufferSize);

        transport.Open(name, adios2::Mode::Read);
        transport.Read(readBuffer, bufferSize, 0);
        transport.Close();

        std::string readData(readBuffer);
        free(readBuffer);

        return readData;
    };

    std::string posixResult = performTestOnTransport(posix);
    std::string flexnvmeResult = performTestOnTransport(flexnvme);

    ASSERT_EQ(std::make_pair(posixResult.length(), posixResult),
              std::make_pair(flexnvmeResult.length(), flexnvmeResult));
}

TEST_F(FileFlexNVMeFilePOSIXMatchTestSuite,
       MultipleWriteAndSingleReadYieldsTheSame)
{
    Rng rng;

    adios2::transport::FilePOSIX posix(adios2::helper::CommDummy());
    adios2::transport::FileFlexNVMe flexnvme(adios2::helper::CommDummy());
    flexnvme.SetParameters(GetParams());

    std::string name = rng.RandString(24, 'a', 'z');

    const size_t NUM_WRITES = 4;
    size_t bufferSize = rng.RandRange(NUM_WRITES * 3, 3 * m_blockSize);
    std::string writeData = rng.RandString(bufferSize - 1);

    std::array<size_t, NUM_WRITES> batchSizes{};
    size_t sizeLeft = bufferSize;

    for (int i = 0; i < NUM_WRITES; i++)
    {
        if (i == NUM_WRITES - 1)
        {
            batchSizes[i] = sizeLeft;
            break;
        }

        size_t subSize = rng.RandRange(1, sizeLeft - 2);
        batchSizes[i] = subSize;
        sizeLeft -= subSize;
    }

    auto performTestOnTransport = [&](adios2::Transport &transport) {
        transport.Open(name, adios2::Mode::Write);

        int startIndex = 0;
        for (auto batchSize : batchSizes)
        {
            std::string batchWriteData =
                writeData.substr(startIndex, batchSize);
            transport.Write(batchWriteData.c_str(), batchSize,
                            adios2::MaxSizeT);
            startIndex += batchSize;
        }

        transport.Close();

        char *readBuffer = (char *)malloc(bufferSize);

        transport.Open(name, adios2::Mode::Read);
        size_t size = transport.GetSize();
        transport.Read(readBuffer, size, adios2::MaxSizeT);
        transport.Close();

        std::string readData(readBuffer);
        free(readBuffer);

        return std::make_pair(size, readData);
    };

    auto posixResult = performTestOnTransport(posix);
    auto flexnvmeResult = performTestOnTransport(flexnvme);

    ASSERT_EQ(posixResult.first, flexnvmeResult.first);
    ASSERT_EQ(
        std::make_pair(posixResult.second.length(), posixResult.second),
        std::make_pair(flexnvmeResult.second.length(), flexnvmeResult.second));
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));
    return RUN_ALL_TESTS();
}
