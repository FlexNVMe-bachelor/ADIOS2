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

    ASSERT_EQ(flexnvmeResult, posixResult);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));
    return RUN_ALL_TESTS();
}
