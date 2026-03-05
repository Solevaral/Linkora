#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "app/auth.h"
#include "app/coordinator_api.h"
#include "app/config.h"
#include "app/port_open.h"
#include "core/tunnel.h"
#include "network/frame.h"
#include "network/route_manager.h"
#include "network/transport.h"
#include "utils/logging.h"

int main(int argc, char **argv)
{
    const std::string configPath = (argc > 1) ? argv[1] : "config/host.yaml";

    linkora::app::HostConfig config;
    std::string error;
    if (!linkora::app::LoadHostConfig(configPath, config, error))
    {
        std::cerr << "Config error: " << error << '\n';
        return 1;
    }

    linkora::network::UdpTransport transport;
    if (!transport.Bind(config.listenHost, config.listenPort))
    {
        std::cerr << "Failed to bind host transport on " << config.listenHost << ':' << config.listenPort << '\n';
        return 1;
    }

    const std::string networkName = !config.networkName.empty() ? config.networkName : config.login;
    if (config.passwordPlain.empty())
    {
        std::cerr << "Coordinator mode requires auth.password in host config\n";
        transport.Close();
        return 1;
    }

    std::string openPortError;
    if (!linkora::app::EnsureUdpPortOpen(config.listenPort, openPortError))
    {
        std::cerr << "Port auto-open warning: " << openPortError << '\n';
        std::cerr << "Host can continue, but clients may fail to connect if firewall blocks UDP "
                  << config.listenPort << ".\n";
    }

    const std::string hostClientId = "host-" + networkName + "-" + std::to_string(config.listenPort);
    if (!linkora::app::CheckCoordinatorReachable(
            transport,
            config.coordinatorHost,
            config.coordinatorPort,
            2500,
            error))
    {
        std::cerr << "Port check failed: " << error << '\n';
        std::cerr << "Hint: open UDP " << config.coordinatorPort << " on coordinator server and allow outbound/inbound UDP on host.\n";
        transport.Close();
        return 1;
    }

    const auto registration = linkora::app::RegisterHostWithCoordinator(
        transport,
        config.coordinatorHost,
        config.coordinatorPort,
        networkName,
        config.passwordPlain,
        hostClientId,
        config.listenPort,
        5000);
    if (!registration.ok)
    {
        std::cerr << "Coordinator registration failed: " << registration.error << '\n';
        transport.Close();
        return 1;
    }

    linkora::utils::Log(linkora::utils::LogLevel::Info, "Network registered in coordinator: " + registration.network);
    linkora::utils::Log(linkora::utils::LogLevel::Info, "Host started, waiting for client auth...");

    linkora::network::RouteManager routeManager;
    auto tunnel = linkora::core::CreatePlatformTunnel();
    const bool useTun = std::getenv("LINKORA_USE_TUN") != nullptr;
    if (useTun)
    {
        if (!linkora::core::CreateAndBringUpTunnel(*tunnel, "linkora0", config.mtu, error))
        {
            std::cerr << "Tunnel setup error: " << error << '\n';
            transport.Close();
            return 1;
        }

        if (!routeManager.Setup(config.virtualSubnet, tunnel->DeviceName(), error))
        {
            std::cerr << "Route setup error: " << error << '\n';
            tunnel->Close();
            transport.Close();
            return 1;
        }

        linkora::utils::Log(linkora::utils::LogLevel::Info, "Tunnel up on device " + tunnel->DeviceName());
    }

    linkora::app::AuthResult auth;
    while (true)
    {
        auth = linkora::app::HostAuthenticate(transport, config, 30000);
        if (auth.ok)
        {
            break;
        }
        std::cerr << "Auth failed: " << auth.error << " (continuing to wait)\n";
    }

    linkora::utils::Log(linkora::utils::LogLevel::Info, "Authenticated peer " + auth.peerHost + ":" + std::to_string(auth.peerPort));

    std::vector<std::uint8_t> frame;
    std::string peerHost;
    std::uint16_t peerPort = 0;
    if (!transport.ReceiveFrom(frame, peerHost, peerPort, 30000))
    {
        std::cerr << "Timeout waiting for encrypted frame\n";
        routeManager.Cleanup();
        tunnel->Close();
        transport.Close();
        return 1;
    }

    linkora::network::WireHeader header;
    std::vector<std::uint8_t> plaintext;
    if (!linkora::network::ParseEncryptedFrame(auth.dataKey, frame, header, plaintext))
    {
        std::cerr << "Failed to parse encrypted frame\n";
        routeManager.Cleanup();
        tunnel->Close();
        transport.Close();
        return 1;
    }

    const std::string msg(plaintext.begin(), plaintext.end());
    linkora::utils::Log(linkora::utils::LogLevel::Info, "Received secure payload: " + msg);

    routeManager.Cleanup();
    tunnel->Close();
    transport.Close();
    return 0;
}
