#pragma once

#include <cstdint>
#include <string>

namespace linkora::app
{

    struct HostConfig
    {
        std::string networkName;
        std::string listenHost;
        std::uint16_t listenPort = 0;
        std::string coordinatorHost;
        std::uint16_t coordinatorPort = 0;
        std::string login;
        std::string passwordHash;
        std::string passwordPlain;
        std::string virtualSubnet;
        int mtu = 1400;
    };

    struct ClientConfig
    {
        std::string host;
        std::uint16_t port = 0;
        std::string login;
        std::string password;
        std::string virtualIp;
        int mtu = 1400;
    };

    bool LoadHostConfig(const std::string &path, HostConfig &outConfig, std::string &error);
    bool LoadClientConfig(const std::string &path, ClientConfig &outConfig, std::string &error);

} // namespace linkora::app
