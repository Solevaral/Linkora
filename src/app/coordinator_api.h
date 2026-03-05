#pragma once

#include <cstdint>
#include <string>

#include "network/transport.h"

namespace linkora::app
{
    struct CoordinatorResult
    {
        bool ok = false;
        std::string network;
        std::string subnet;
        std::string assignedIp;
        std::string hostIp;
        std::uint16_t hostPort = 0;
        std::string error;
    };

    bool CheckCoordinatorReachable(
        network::UdpTransport &transport,
        const std::string &coordinatorHost,
        std::uint16_t coordinatorPort,
        int timeoutMs,
        std::string &error);

    CoordinatorResult RegisterHostWithCoordinator(
        network::UdpTransport &transport,
        const std::string &coordinatorHost,
        std::uint16_t coordinatorPort,
        const std::string &networkName,
        const std::string &password,
        const std::string &hostClientId,
        std::uint16_t hostDataPort,
        int timeoutMs);

    CoordinatorResult JoinNetworkViaCoordinator(
        network::UdpTransport &transport,
        const std::string &coordinatorHost,
        std::uint16_t coordinatorPort,
        const std::string &networkName,
        const std::string &password,
        const std::string &clientId,
        int timeoutMs);
}
