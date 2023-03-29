/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDescriptor.h wrapper of xNVME library functions for file I/O
 *
 *  Created on: Oct 6, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_FILE_FLEXNVME_H_
#define ADIOS2_TOOLKIT_TRANSPORT_FILE_FLEXNVME_H_

#include <cstdint>
#include <future> //std::async, std::future

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/toolkit/transport/Transport.h"

#include <flan.h>

namespace adios2
{
namespace helper
{
class Comm;
}
namespace transport
{

/** File descriptor transport using the xNVME IO library */
class FileFlexNVMe : public Transport
{

public:
    explicit FileFlexNVMe(helper::Comm const &comm);

    ~FileFlexNVMe() noexcept;

    void SetParameters(const Params &parameters);

    void Open(const std::string &name, const Mode openMode,
              const bool async = false, const bool directio = false) final;

    void OpenChain(const std::string &name, Mode openMode,
                   const helper::Comm &chainComm, const bool async = false,
                   const bool directio = false) final;

    void Write(const char *buffer, size_t size, size_t start = MaxSizeT) final;

#ifdef REALLY_WANT_WRITEV
    /* Actual writev() function, inactive for now */
    void WriteV(const core::iovec *iov, const int iovcnt,
                size_t start = MaxSizeT) final;
#endif

    void Read(char *buffer, size_t size, size_t start = MaxSizeT) final;

    size_t GetSize() final;

    /** Does nothing, each write is supposed to flush */
    void Flush() final;

    void Close() final;

    void Delete() final;

    void SeekToEnd() final;

    void SeekToBegin() final;

    void Seek(const size_t start = MaxSizeT) final;

    void Truncate(const size_t length) final;

    void MkDir(const std::string &fileName) final;

    std::string IncrementChunkName();

private:
    std::string m_deviceUrl = "";
    std::string m_baseName = "";
    std::string m_poolName;

    size_t m_chunkWrites = 0;
    size_t m_objectSize = 0;

    static struct flan_handle *flanh;
    static int refCount;

    auto ErrnoErrMsg() const -> std::string;

    void InitFlan(const std::string &name);
    auto NormalisedObjectName(std::string &input) -> std::string;
    auto GenerateChunkName(size_t chunkNum) -> std::string;
    auto OpenFlanObject(std::string &objectName) -> uint64_t;
    void CloseFlanObject(uint64_t objectHandle);
};

} // end namespace transport
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORT_FILE_FLEXNVME_H_ */
