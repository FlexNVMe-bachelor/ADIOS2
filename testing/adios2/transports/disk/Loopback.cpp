#include "Loopback.h"

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <linux/loop.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <utility>

namespace Disk
{
LoopbackDevice::LoopbackDevice(std::string backingFilePath)
: m_BackingFileName(std::move(backingFilePath))
{
}

LoopbackDevice::LoopbackDevice() : m_BackingFileName("") {}

LoopbackDevice::~LoopbackDevice() { Teardown(); }

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

void LoopbackDevice::Teardown()
{
    int err = 0;
    if (m_LdFileDescriptor != -1)
    {
        err = ioctl(m_LdFileDescriptor, LOOP_CLR_FD);
        if (err == -1)
        {
            std::cerr << "Failed to delete loopback device\n";
        }
    }

    if (m_BackingFileDescriptor != -1)
    {
        err = close(m_BackingFileDescriptor);
        if (err)
        {
            std::cerr << "Failed to close backingfile, errcode: "
                      << std::to_string(err) << "\n";
        }

        m_BackingFileDescriptor = -1;
    }

    if (m_LdFileDescriptor != -1)
    {
        err = close(m_LdFileDescriptor);
        if (err)
        {
            std::cerr << "Failed to close loopback device, errcode: "
                      << std::to_string(err) << "\n";
        }

        m_LdFileDescriptor = -1;
        m_DeviceNr = -1;
    }

    if (m_LdctlFileDescriptor != -1)
    {
        err = close(m_LdctlFileDescriptor);
        if (err)
        {
            std::cerr << "Failed to close loop control, errcode: "
                      << std::to_string(err) << "\n";
        }

        m_LdctlFileDescriptor = -1;
    }
}

void LoopbackDevice::GetCtlFileDescriptor()
{
    m_LdctlFileDescriptor = open("/dev/loop-control", O_RDWR);
    if (m_LdctlFileDescriptor == -1)
    {
        throw std::runtime_error("Failed to open /dev/loop-control");
    }
}

void LoopbackDevice::GetDeviceNr()
{
    m_DeviceNr = ioctl(m_LdctlFileDescriptor, LOOP_CTL_GET_FREE);
    if (m_DeviceNr == -1)
    {
        throw std::runtime_error("Could not get a free loopback device");
    }
}

void LoopbackDevice::CreateLoopName()
{
    m_LoopName = "/dev/loop" + std::to_string(m_DeviceNr);
}

void LoopbackDevice::OpenDevice()
{

    m_LdFileDescriptor = open(m_LoopName.c_str(), O_RDWR);
    if (m_LdFileDescriptor == -1)
    {
        throw std::runtime_error("Failed to open loopback device: " +
                                 m_LoopName);
    }
}

void LoopbackDevice::OpenBackingFile()
{
    m_BackingFileDescriptor = open(m_BackingFileName.c_str(), O_RDWR);
    if (m_BackingFileDescriptor == -1)
    {
        throw std::runtime_error("Failed to open backingfile:  " +
                                 m_BackingFileName);
    }
}

void LoopbackDevice::LinkLoopbackAndFile()
{
    if (ioctl(m_LdFileDescriptor, LOOP_SET_FD, m_BackingFileDescriptor) == -1)
    {
        throw std::runtime_error(
            "Failed to link loopback device to backingfile");
    }
}
} // namespace Utils
