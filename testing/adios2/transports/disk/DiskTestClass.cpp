#include "DiskTestClass.h"

#include "DiskTestClass.h"
#include "adios2/common/ADIOSTypes.h"
#include "mkfs.h"

namespace Disk
{
BaseDiskTestClass::BaseDiskTestClass(size_t blockSize, size_t blockCount,
                                     size_t poolCount)
: m_blockSize(blockSize), m_blockCount(blockCount), m_poolCount(poolCount)
{
    Disk::BackingFile bf(m_blockSize, m_blockCount);
    backingFile = bf;
    backingFile.Create();

    Disk::LoopbackDevice dev(backingFile.GetPath());
    device = dev;
    device.Create();

    mkfs(device.GetDeviceUrl(), m_poolCount);
}

auto BaseDiskTestClass::GetParams() -> adios2::Params
{
    return {{"Library", "flexnvme"},
            {"device_url", device.GetDeviceUrl()},
            {"object_size", std::to_string(m_blockSize)},
            {"pool_name", "mypool"}};
}
} // namespace Disk
