#include "app/coordinator_api.h"

#include <sstream>
#include <vector>

namespace linkora::app
{
    namespace
    {
        std::vector<std::string> Split(const std::string &line, char sep)
        {
            std::vector<std::string> parts;
            std::stringstream ss(line);
            std::string item;
            while (std::getline(ss, item, sep))
            {
                parts.push_back(item);
            }
            return parts;
        }

        bool ParsePort(const std::string &value, std::uint16_t &outPort)
        {
            try
            {
                const int parsed = std::stoi(value);
                if (parsed < 0 || parsed > 65535)
                {
                    return false;
                }
                outPort = static_cast<std::uint16_t>(parsed);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        CoordinatorResult ExecuteCoordinatorCommand(
            network::UdpTransport &transport,
            const std::string &coordinatorHost,
            std::uint16_t coordinatorPort,
            const std::string &command,
            int timeoutMs)
        {
            CoordinatorResult result;
            if (!transport.SendTo(
                    coordinatorHost,
                    coordinatorPort,
                    std::vector<std::uint8_t>(command.begin(), command.end())))
            {
                result.error = "Failed to send command to coordinator";
                return result;
            }

            std::vector<std::uint8_t> packet;
            std::string peerHost;
            std::uint16_t peerPort = 0;
            if (!transport.ReceiveFrom(packet, peerHost, peerPort, timeoutMs))
            {
                result.error = "Timeout waiting for coordinator response";
                return result;
            }

            const std::string response(packet.begin(), packet.end());
            const auto parts = Split(response, '|');
            if (parts.size() < 2)
            {
                result.error = "Invalid coordinator response";
                return result;
            }

            if (parts[0] == "ERR")
            {
                result.error = parts[1];
                return result;
            }

            if (parts[0] != "OK")
            {
                result.error = "Unknown coordinator response";
                return result;
            }

            if (parts.size() < 5)
            {
                result.error = "Coordinator response too short";
                return result;
            }

            result.network = parts[2];
            result.subnet = parts[3];
            result.assignedIp = parts[4];

            if (parts.size() >= 7)
            {
                result.hostIp = parts[5];
                if (!ParsePort(parts[6], result.hostPort))
                {
                    result.error = "Invalid host port from coordinator";
                    return result;
                }
            }

            result.ok = true;
            return result;
        }
    }

    CoordinatorResult RegisterHostWithCoordinator(
        network::UdpTransport &transport,
        const std::string &coordinatorHost,
        std::uint16_t coordinatorPort,
        const std::string &networkName,
        const std::string &password,
        const std::string &hostClientId,
        std::uint16_t hostDataPort,
        int timeoutMs)
    {
        const std::string command =
            "CREATE|" + networkName + "|" + password + "|" + hostClientId + "|" + std::to_string(hostDataPort);
        return ExecuteCoordinatorCommand(transport, coordinatorHost, coordinatorPort, command, timeoutMs);
    }

    bool CheckCoordinatorReachable(
        network::UdpTransport &transport,
        const std::string &coordinatorHost,
        std::uint16_t coordinatorPort,
        int timeoutMs,
        std::string &error)
    {
        const std::string ping = "PING";
        if (!transport.SendTo(
                coordinatorHost,
                coordinatorPort,
                std::vector<std::uint8_t>(ping.begin(), ping.end())))
        {
            error = "Cannot send to coordinator. Check network.host/network.port and firewall.";
            return false;
        }

        std::vector<std::uint8_t> packet;
        std::string peerHost;
        std::uint16_t peerPort = 0;
        if (!transport.ReceiveFrom(packet, peerHost, peerPort, timeoutMs))
        {
            error = "Coordinator UDP port is not reachable. Open UDP port and check firewall/NAT.";
            return false;
        }

        const std::string response(packet.begin(), packet.end());
        if (response != "OK|pong")
        {
            error = "Unexpected response from coordinator during port check: " + response;
            return false;
        }

        return true;
    }

    CoordinatorResult JoinNetworkViaCoordinator(
        network::UdpTransport &transport,
        const std::string &coordinatorHost,
        std::uint16_t coordinatorPort,
        const std::string &networkName,
        const std::string &password,
        const std::string &clientId,
        int timeoutMs)
    {
        const std::string command = "JOIN|" + networkName + "|" + password + "|" + clientId;
        return ExecuteCoordinatorCommand(transport, coordinatorHost, coordinatorPort, command, timeoutMs);
    }
}
