#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace linkora::core
{

    class ITunnel
    {
    public:
        virtual ~ITunnel() = default;

        virtual bool Open(const char *interfaceName) = 0;
        virtual int Read(std::vector<std::uint8_t> &outPacket) = 0;
        virtual int Write(const std::vector<std::uint8_t> &packet) = 0;
        virtual const std::string &DeviceName() const = 0;
        virtual void Close() = 0;
    };

    std::unique_ptr<ITunnel> CreatePlatformTunnel();

    bool CreateAndBringUpTunnel(
        ITunnel &tunnel,
        const char *requestedName,
        int mtu,
        std::string &error);

} // namespace linkora::core
