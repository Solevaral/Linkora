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
        void Close();

    private:
        bool isOpen_ = false;
    };

} // namespace linkora::network
