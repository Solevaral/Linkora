#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "app/auth.h"
#include "app/coordinator_api.h"
#include "app/config.h"
#include "core/tunnel.h"
#include "network/frame.h"
#include "network/route_manager.h"
#include "network/transport.h"
#include "utils/logging.h"

#include <chrono>

int main(int argc, char **argv)
{
    const std::string configPath = (argc > 1) ? argv[1] : "config/client.yaml";

    linkora::app::ClientConfig config;
    std::string error;
    if (!linkora::app::LoadClientConfig(configPath, config, error))
    {
        std::cerr << "Config error: " << error << '\n';
        return 1;
    }

    linkora::network::UdpTransport transport;
    if (!transport.Bind("0.0.0.0", 0))
    {
        std::cerr << "Failed to initialize client transport\n";
        return 1;
    }

    linkora::network::RouteManager routeManager;
    auto tunnel = linkora::core::CreatePlatformTunnel();
    const bool useTun = std::getenv("LINKORA_USE_TUN") != nullptr;
    if (useTun && !linkora::core::CreateAndBringUpTunnel(*tunnel, "linkora1", config.mtu, error))
    {
        std::cerr << "Tunnel setup error: " << error << '\n';
        transport.Close();
        return 1;
    }

    const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
    const std::string networkName = config.login;
    const std::string clientId = "client-" + std::to_string(nowMs);
    const auto join = linkora::app::JoinNetworkViaCoordinator(
        transport,
        config.host,
        config.port,
        networkName,
        config.password,
        clientId,
        5000);
    if (!join.ok)
    {
        std::cerr << "Join failed: " << join.error << '\n';
        routeManager.Cleanup();
        tunnel->Close();
        transport.Close();
        return 1;
    }

    linkora::utils::Log(
        linkora::utils::LogLevel::Info,
        "Joined network " + join.network + ", assigned IP " + join.assignedIp + ", host endpoint " + join.hostIp + ":" + std::to_string(join.hostPort));

    linkora::app::ClientConfig hostAuthConfig = config;
    hostAuthConfig.host = join.hostIp;
    hostAuthConfig.port = join.hostPort;
    hostAuthConfig.virtualIp = join.assignedIp;

    auto auth = linkora::app::ClientAuthenticate(transport, hostAuthConfig, 10000);
    if (!auth.ok)
    {
        std::cerr << "Auth failed: " << auth.error << '\n';
        routeManager.Cleanup();
        tunnel->Close();
        transport.Close();
        return 1;
    }

    linkora::utils::Log(linkora::utils::LogLevel::Info, "Authenticated with session id " + std::to_string(auth.sessionId));

    if (useTun)
    {
        const std::string assignedIp = !join.assignedIp.empty() ? join.assignedIp : (!auth.virtualIp.empty() ? auth.virtualIp : config.virtualIp);
        if (assignedIp.empty())
        {
            std::cerr << "Auth succeeded but no virtual IP was assigned\n";
            routeManager.Cleanup();
            tunnel->Close();
            transport.Close();
            return 1;
        }

        if (!routeManager.Setup(assignedIp + "/32", tunnel->DeviceName(), error))
        {
            std::cerr << "Route setup error: " << error << '\n';
            routeManager.Cleanup();
            tunnel->Close();
            transport.Close();
            return 1;
        }

        linkora::utils::Log(linkora::utils::LogLevel::Info, "Tunnel up on device " + tunnel->DeviceName() + ", virtual IP " + assignedIp);
    }

    linkora::network::WireHeader header;
    header.sessionId = auth.sessionId;
    header.counter = 1;

    const std::vector<std::uint8_t> message = {'p', 'i', 'n', 'g', ' ', 'f', 'r', 'o', 'm', ' ', 'c', 'l', 'i', 'e', 'n', 't'};
    std::vector<std::uint8_t> frame;
    if (!linkora::network::BuildEncryptedFrame(auth.dataKey, header, message, frame))
    {
        std::cerr << "Failed to build encrypted frame\n";
        routeManager.Cleanup();
        tunnel->Close();
        transport.Close();
        return 1;
    }

    if (!transport.SendTo(hostAuthConfig.host, hostAuthConfig.port, frame))
    {
        std::cerr << "Failed to send encrypted frame\n";
        routeManager.Cleanup();
        tunnel->Close();
        transport.Close();
        return 1;
    }

    linkora::utils::Log(linkora::utils::LogLevel::Info, "Encrypted payload sent to host");
    routeManager.Cleanup();
    tunnel->Close();
    transport.Close();
    return 0;
}
