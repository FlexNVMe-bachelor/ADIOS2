#include "Loopback.h"

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <linux/loop.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <utility>

#define errExit(msg)                                                           \
    do                                                                         \
    {                                                                          \
        std::cerr << msg << "\n";                                              \
        exit(EXIT_FAILURE);                                                    \
    } while (0)

namespace Disk
{
LoopbackDevice::LoopbackDevice(std::string backingFilePath)
: m_BackingFileName(std::move(backingFilePath))
{
}

LoopbackDevice::LoopbackDevice() { m_BackingFileName = ""; }

auto LoopbackDevice::Create() -> std::string
{
    GetCtlFileDescriptor();
    GetDeviceNr();
    CreateLoopName();
    OpenDevice();
    OpenBackingFile();
    LinkLoopbackAndFile();

    return GetDeviceUrl();
}

auto LoopbackDevice::GetDeviceUrl() -> std::string { return m_LoopName; }

void LoopbackDevice::GetCtlFileDescriptor()
{
    m_LdctlFileDescriptor = open("/dev/loop-control", O_RDWR);
    if (m_LdctlFileDescriptor == -1)
    {
        errExit("Failed to open /dev/loop-control");
    }
}

void LoopbackDevice::GetDeviceNr()
{
    m_DeviceNr = ioctl(m_LdctlFileDescriptor, LOOP_CTL_GET_FREE);
    if (m_DeviceNr == -1)
    {
        errExit("Could not get a free loopback device");
    }
}

void LoopbackDevice::CreateLoopName()
{
    m_LoopName = "/dev/loop" + std::to_string(m_DeviceNr);
    std::cout << "Using: " << m_LoopName << "\n";
}

void LoopbackDevice::OpenDevice()
{

    m_LdFileDescriptor = open(m_LoopName.c_str(), O_RDWR);
    if (m_LdFileDescriptor == -1)
    {
        errExit("Failed to open loopback device: " + m_LoopName);
    }
}

void LoopbackDevice::OpenBackingFile()
{
    m_BackingFileDescriptor = open(m_BackingFileName.c_str(), O_RDWR);
    if (m_BackingFileDescriptor == -1)
    {
        errExit("Failed to open backingfile:  " + m_BackingFileName);
    }
}

void LoopbackDevice::LinkLoopbackAndFile()
{
    if (ioctl(m_LdFileDescriptor, LOOP_SET_FD, m_BackingFileDescriptor) == -1)
    {
        errExit("Failed to link");
    }
}

} // namespace Utils
