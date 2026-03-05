#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "app/auth.h"
#include "app/config.h"
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

    auto auth = linkora::app::HostAuthenticate(transport, config, 30000);
    if (!auth.ok)
    {
        std::cerr << "Auth failed: " << auth.error << '\n';
        routeManager.Cleanup();
        tunnel->Close();
        transport.Close();
        return 1;
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
