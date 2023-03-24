#ifndef DISKCLASS_H
#define DISKCLASS_H

#include <gtest/gtest.h>

#include "adios2/common/ADIOSTypes.h"

#include "BackingFile.h"
#include "Loopback.h"
#include <cstddef>

namespace Disk
{
class BaseDiskTestClass
{
public:
    LoopbackDevice device;
    BackingFile backingFile;

    BaseDiskTestClass(size_t blockSize, size_t blockCount, size_t poolCount);

    auto GetParams() -> adios2::Params;

protected:
    size_t m_blockSize;
    size_t m_blockCount;
    size_t m_poolCount;
};

class DiskTestClass : public BaseDiskTestClass, public ::testing::Test
{
    using BaseDiskTestClass::BaseDiskTestClass;
};

template <typename T>
class DiskTestClassWithParams : public BaseDiskTestClass,
                                public ::testing::TestWithParam<T>
{
    using BaseDiskTestClass::BaseDiskTestClass;
};
} // namespace Disk
  //
#endif
