#include <algorithm>
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

TEST_F(FileFlexNVMeFilePOSIXMatchTestSuite,
       SingleWriteAndMultipleReadYieldsTheSame)
{
    Rng rng;

    adios2::transport::FilePOSIX posix(adios2::helper::CommDummy());
    adios2::transport::FileFlexNVMe flexnvme(adios2::helper::CommDummy());
    flexnvme.SetParameters(GetParams());

    std::string name = rng.RandString(24, 'a', 'z');

    const size_t NUM_READS = 4;
    size_t bufferSize = rng.RandRange(NUM_READS * 3, 3 * m_blockSize);
    std::string writeData = rng.RandString(bufferSize - 1);

    std::array<size_t, NUM_READS> batchSizes{};
    size_t sizeLeft = bufferSize;

    for (int i = 0; i < NUM_READS; i++)
    {
        if (i == NUM_READS - 1)
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
        transport.Write(writeData.c_str(), bufferSize, adios2::MaxSizeT);
        transport.Close();

        transport.Open(name, adios2::Mode::Read);

        std::vector<std::tuple<size_t, size_t, std::string>> batchResults;
        for (auto batchSize : batchSizes)
        {
            char *readBuffer = (char *)malloc(batchSize + 1);
            readBuffer[batchSize] = '\0';

            size_t size = transport.GetSize();
            transport.Read(readBuffer, batchSize, adios2::MaxSizeT);

            std::string readData(readBuffer);
            free(readBuffer);

            auto batchResult =
                std::make_tuple(size, readData.length(), readData);
            batchResults.push_back(batchResult);
        }

        transport.Close();

        return batchResults;
    };

    auto posixResult = performTestOnTransport(posix);
    auto flexnvmeResult = performTestOnTransport(flexnvme);

    ASSERT_EQ(posixResult, flexnvmeResult);
}

TEST_F(FileFlexNVMeFilePOSIXMatchTestSuite, RandomWriteReadsYieldsTheSame)
{
    Rng rng;

    adios2::transport::FilePOSIX posix(adios2::helper::CommDummy());
    adios2::transport::FileFlexNVMe flexnvme(adios2::helper::CommDummy());
    flexnvme.SetParameters(GetParams());

    std::string name = rng.RandString(24, 'a', 'z');

    const size_t MAX_BLOCKS = 24, MAX_NUM_ACTIONS = 1000;
    const size_t numActions = rng.RandRange(5, MAX_NUM_ACTIONS);
    const size_t bufferSize =
        rng.RandRange(numActions * 2, MAX_BLOCKS * m_blockSize);

    const std::string writeData = rng.RandString(bufferSize - 1);

    std::vector<std::vector<std::pair<size_t, size_t>>> batches{};

    size_t sizeLeft = bufferSize;

    std::vector<std::pair<size_t, size_t>> curBatch;

    for (int i = 0; i < numActions; i++)
    {
        bool shouldSpecifyOffset = rng.RandRange(1, 4) == 1;
        size_t offset = shouldSpecifyOffset ? rng.RandRange(0, bufferSize / 2)
                                            : adios2::MaxSizeT;

        if (shouldSpecifyOffset && !curBatch.empty())
        {
            batches.push_back(curBatch);
            std::vector<std::pair<size_t, size_t>> newBatch;
            curBatch = newBatch;
        }

        if (i == numActions - 1)
        {
            curBatch.emplace_back(sizeLeft, offset);
            batches.push_back(curBatch);
            break;
        }

        size_t subSize = rng.RandRange(0, bufferSize / numActions);

        curBatch.emplace_back(subSize, offset);

        sizeLeft -= subSize;
    }

    auto runBatchOnTransport = [&](const std::vector<std::pair<size_t, size_t>>
                                       &batch,
                                   adios2::Transport &transport) {
        std::vector<
            std::tuple<std::pair<size_t, size_t>, size_t, size_t, std::string>>
            batchResults;

        transport.Open(name, adios2::Mode::Write);

        size_t startIndex = 0;
        size_t writtenSize = 0;
        for (auto batchItem : batch)
        {
            size_t batchSize = batchItem.first, batchOffset = batchItem.second;
            startIndex =
                batchOffset == adios2::MaxSizeT ? startIndex : batchOffset;

            std::string batchWriteData =
                writeData.substr(startIndex, batchSize);

            transport.Write(batchWriteData.c_str(), batchSize, batchOffset);

            startIndex += batchSize;
            writtenSize += batchSize;
        }

        transport.Close();

        transport.Open(name, adios2::Mode::Read);

        for (auto batchItem : batch)
        {
            size_t batchSize = batchItem.first, batchOffset = batchItem.second;

            char *readBuffer = (char *)malloc(batchSize + 1);
            readBuffer[batchSize] = '\0';

            size_t size = transport.GetSize();

            transport.Read(readBuffer, batchSize, batchOffset);

            std::string readData(readBuffer);
            free(readBuffer);

            auto batchResult =
                std::make_tuple(std::make_pair(batchSize, batchOffset),
                                batchSize, readData.length(), readData);

            batchResults.push_back(batchResult);
        }

        transport.Close();

        return batchResults;
    };

    for (const auto &batch : batches)
    {
        auto posixResult = runBatchOnTransport(batch, posix);
        auto flexnvmeResult = runBatchOnTransport(batch, flexnvme);

        ASSERT_EQ(posixResult.size(), flexnvmeResult.size());
        ASSERT_EQ(posixResult, flexnvmeResult);
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));
    return RUN_ALL_TESTS();
}
