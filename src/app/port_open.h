#pragma once

#include <cstdint>
#include <string>

namespace linkora::app
{
    // Tries to open inbound UDP port in local firewall.
    // Returns true on success or when no known firewall tool is found.
    bool EnsureUdpPortOpen(std::uint16_t port, std::string &error);
}
