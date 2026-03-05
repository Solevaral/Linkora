#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "app/config.h"
#include "network/transport.h"

namespace linkora::app
{

    struct AuthResult
    {
        bool ok = false;
        std::uint64_t sessionId = 0;
        std::vector<std::uint8_t> dataKey;
        std::string virtualIp;
        std::string virtualSubnet;
        std::string peerHost;
        std::uint16_t peerPort = 0;
        std::string error;
    };

    AuthResult HostAuthenticate(
        network::UdpTransport &transport,
        const HostConfig &config,
        int timeoutMs);

    AuthResult ClientAuthenticate(
        network::UdpTransport &transport,
        const ClientConfig &config,
        int timeoutMs);

    bool SendRelayPacket(
        network::UdpTransport &transport,
        const std::string &coordinatorHost,
        std::uint16_t coordinatorPort,
        const std::string &targetHost,
        std::uint16_t targetPort,
        const std::vector<std::uint8_t> &payload);

} // namespace linkora::app
