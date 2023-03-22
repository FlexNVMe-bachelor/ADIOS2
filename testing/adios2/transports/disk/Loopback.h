#ifndef LOOPBACK_DEVICE_H
#define LOOPBACK_DEVICE_H

#include <string>

namespace Disk
{
class LoopbackDevice
{
public:
    explicit LoopbackDevice(std::string backingFilePath);
    explicit LoopbackDevice();
    ~LoopbackDevice();

    auto Create() -> std::string;
    auto GetDeviceUrl() -> std::string;

    void Teardown();

private:
    int m_LdctlFileDescriptor = -1;
    int m_LdFileDescriptor = -1;
    int m_BackingFileDescriptor = -1;

    long m_DeviceNr = -1;

    std::string m_BackingFileName;
    std::string m_LoopName;

    void GetCtlFileDescriptor();
    void GetDeviceNr();
    void CreateLoopName();
    void OpenDevice();
    void OpenBackingFile();
    void LinkLoopbackAndFile();
};
} // namespace Utils

#endif
