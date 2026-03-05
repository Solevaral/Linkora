#include "network/transport.h"

namespace linkora::network
{

    bool UdpTransport::Bind(const std::string &host, std::uint16_t port)
    {
        (void)host;
        (void)port;
        isOpen_ = true;
        return true;
    }

    bool UdpTransport::SendTo(
        const std::string &host,
        std::uint16_t port,
        const std::vector<std::uint8_t> &payload)
    {
        (void)host;
        (void)port;
        (void)payload;
        return isOpen_;
    }

    void UdpTransport::Close()
    {
        isOpen_ = false;
    }

} // namespace linkora::network
