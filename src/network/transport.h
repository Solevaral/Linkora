#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace linkora::network
{

    class UdpTransport
    {
    public:
        bool Bind(const std::string &host, std::uint16_t port);
        bool SendTo(const std::string &host, std::uint16_t port, const std::vector<std::uint8_t> &payload);
        bool ReceiveFrom(
            std::vector<std::uint8_t> &payload,
            std::string &peerHost,
            std::uint16_t &peerPort,
            int timeoutMs);
        bool IsOpen() const;
        void Close();

    private:
        int socketFd_ = -1;
    };

} // namespace linkora::network
