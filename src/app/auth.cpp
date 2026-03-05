#include "app/auth.h"

#include <openssl/rand.h>

#include <algorithm>
#include <cstdint>
#include <sstream>

#include "core/crypto.h"
#include "core/password.h"

namespace linkora::app
{
    namespace
    {
        constexpr int kKeyIterations = 120000;

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

        std::string ToHex(const std::vector<std::uint8_t> &bytes)
        {
            static const char *kHex = "0123456789abcdef";
            std::string out;
            out.reserve(bytes.size() * 2);
            for (std::uint8_t byte : bytes)
            {
                out.push_back(kHex[(byte >> 4) & 0xF]);
                out.push_back(kHex[byte & 0xF]);
            }
            return out;
        }

        std::vector<std::uint8_t> BuildRelayPayload(
            const std::string &targetHost,
            std::uint16_t targetPort,
            const std::vector<std::uint8_t> &payload)
        {
            const std::string line =
                "RELAY|" + targetHost + "|" + std::to_string(targetPort) + "|" + ToHex(payload);
            return std::vector<std::uint8_t>(line.begin(), line.end());
        }

        bool SendMaybeRelay(
            network::UdpTransport &transport,
            const ClientConfig &config,
            const std::vector<std::uint8_t> &payload)
        {
            if (config.useRelay)
            {
                if (config.relayTargetHost.empty() || config.relayTargetPort == 0)
                {
                    return false;
                }
                return transport.SendTo(
                    config.host,
                    config.port,
                    BuildRelayPayload(config.relayTargetHost, config.relayTargetPort, payload));
            }

            return transport.SendTo(config.host, config.port, payload);
        }

        bool FromHex(const std::string &hex, std::vector<std::uint8_t> &out)
        {
            if (hex.size() % 2 != 0)
            {
                return false;
            }

            auto hexValue = [](char ch) -> int
            {
                if (ch >= '0' && ch <= '9')
                {
                    return ch - '0';
                }
                if (ch >= 'a' && ch <= 'f')
                {
                    return 10 + (ch - 'a');
                }
                if (ch >= 'A' && ch <= 'F')
                {
                    return 10 + (ch - 'A');
                }
                return -1;
            };

            out.clear();
            out.reserve(hex.size() / 2);
            for (std::size_t i = 0; i < hex.size(); i += 2)
            {
                const int hi = hexValue(hex[i]);
                const int lo = hexValue(hex[i + 1]);
                if (hi < 0 || lo < 0)
                {
                    return false;
                }
                out.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
            }
            return true;
        }

        std::uint64_t RandomU64()
        {
            std::uint64_t value = 0;
            (void)RAND_bytes(reinterpret_cast<unsigned char *>(&value), sizeof(value));
            return value;
        }

        bool ParseSubnetPrefix24(const std::string &cidr, std::uint8_t &a, std::uint8_t &b, std::uint8_t &c)
        {
            const auto slashPos = cidr.find('/');
            if (slashPos == std::string::npos || cidr.substr(slashPos + 1) != "24")
            {
                return false;
            }

            const auto ip = cidr.substr(0, slashPos);
            const auto parts = Split(ip, '.');
            if (parts.size() != 4)
            {
                return false;
            }

            auto parseOctet = [](const std::string &s, std::uint8_t &out) -> bool
            {
                if (s.empty() || s.size() > 3)
                {
                    return false;
                }
                try
                {
                    const int v = std::stoi(s);
                    if (v < 0 || v > 255)
                    {
                        return false;
                    }
                    out = static_cast<std::uint8_t>(v);
                    return true;
                }
                catch (...)
                {
                    return false;
                }
            };

            std::uint8_t d = 0;
            if (!parseOctet(parts[0], a) || !parseOctet(parts[1], b) || !parseOctet(parts[2], c) || !parseOctet(parts[3], d))
            {
                return false;
            }

            return d == 0;
        }

        std::string BuildIp(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d)
        {
            return std::to_string(a) + "." + std::to_string(b) + "." + std::to_string(c) + "." + std::to_string(d);
        }

        std::string AssignVirtualIp(const std::string &subnetCidr, const std::string &login)
        {
            std::uint8_t a = 10;
            std::uint8_t b = 44;
            std::uint8_t c = 0;
            if (!ParseSubnetPrefix24(subnetCidr, a, b, c))
            {
                return "10.44.0.10";
            }

            std::uint32_t hash = 2166136261u;
            for (unsigned char ch : login)
            {
                hash ^= static_cast<std::uint32_t>(ch);
                hash *= 16777619u;
            }

            std::uint8_t hostOctet = static_cast<std::uint8_t>(10 + (hash % 230u));
            if (hostOctet == 255)
            {
                hostOctet = 254;
            }
            return BuildIp(a, b, c, hostOctet);
        }
    } // namespace

    AuthResult HostAuthenticate(
        network::UdpTransport &transport,
        const HostConfig &config,
        int timeoutMs)
    {
        AuthResult result;
        std::vector<std::uint8_t> packet;
        std::string peerHost;
        std::uint16_t peerPort = 0;
        if (!transport.ReceiveFrom(packet, peerHost, peerPort, timeoutMs))
        {
            result.error = "Timeout waiting for HELLO";
            return result;
        }

        const std::string hello(packet.begin(), packet.end());
        const auto helloParts = Split(hello, '|');
        if (helloParts.size() != 2 || helloParts[0] != "HELLO")
        {
            result.error = "Invalid HELLO format";
            return result;
        }

        if (helloParts[1] != config.login)
        {
            result.error = "Unknown login";
            return result;
        }

        if (!transport.SendTo(peerHost, peerPort, std::vector<std::uint8_t>{'C', 'H', 'A', 'L', 'L', 'E', 'N', 'G', 'E'}))
        {
            result.error = "Failed to send CHALLENGE";
            return result;
        }

        packet.clear();
        if (!transport.ReceiveFrom(packet, peerHost, peerPort, timeoutMs))
        {
            result.error = "Timeout waiting for AUTH";
            return result;
        }

        const std::string auth(packet.begin(), packet.end());
        const auto authParts = Split(auth, '|');
        if (authParts.size() != 3 || authParts[0] != "AUTH")
        {
            result.error = "Invalid AUTH format";
            return result;
        }

        if (authParts[1] != config.login)
        {
            result.error = "Login mismatch";
            return result;
        }

        bool authOk = false;
        std::string verifyError;
        if (!config.passwordHash.empty())
        {
            authOk = core::VerifyPasswordHash(authParts[2], config.passwordHash, verifyError);
        }
        else
        {
            authOk = (authParts[2] == config.passwordPlain);
            if (!authOk)
            {
                verifyError = "Wrong password";
            }
        }

        if (!authOk)
        {
            const std::string fail = "AUTH_FAIL|" + verifyError;
            (void)transport.SendTo(peerHost, peerPort, std::vector<std::uint8_t>(fail.begin(), fail.end()));
            result.error = verifyError;
            return result;
        }

        std::vector<std::uint8_t> keySalt = {'l', 'i', 'n', 'k', 'o', 'r', 'a'};
        result.dataKey = core::Aes256Gcm::DeriveKeyPbkdf2(authParts[2], keySalt, kKeyIterations);
        if (result.dataKey.empty())
        {
            result.error = "Failed to derive data key";
            return result;
        }

        result.sessionId = RandomU64();
        result.peerHost = peerHost;
        result.peerPort = peerPort;
        result.virtualSubnet = config.virtualSubnet;
        result.virtualIp = AssignVirtualIp(config.virtualSubnet, authParts[1]);

        const std::string ok =
            "AUTH_OK|" + std::to_string(result.sessionId) + "|" + ToHex(result.dataKey) + "|" + result.virtualIp + "|" + result.virtualSubnet;
        if (!transport.SendTo(peerHost, peerPort, std::vector<std::uint8_t>(ok.begin(), ok.end())))
        {
            result.error = "Failed to send AUTH_OK";
            return result;
        }

        result.ok = true;
        return result;
    }

    AuthResult ClientAuthenticate(
        network::UdpTransport &transport,
        const ClientConfig &config,
        int timeoutMs)
    {
        AuthResult result;

        std::vector<std::uint8_t> packet;
        std::string peerHost;
        std::uint16_t peerPort = 0;

        const std::string hello = "HELLO|" + config.login;
        constexpr int kHelloAttempts = 5;
        const int perAttemptTimeoutMs = std::max(1000, timeoutMs / kHelloAttempts);

        bool gotChallenge = false;
        for (int attempt = 0; attempt < kHelloAttempts; ++attempt)
        {
            if (!SendMaybeRelay(transport, config, std::vector<std::uint8_t>(hello.begin(), hello.end())))
            {
                result.error = "Failed to send HELLO";
                return result;
            }

            packet.clear();
            if (transport.ReceiveFrom(packet, peerHost, peerPort, perAttemptTimeoutMs))
            {
                gotChallenge = true;
                break;
            }
        }

        if (!gotChallenge)
        {
            result.error = "Timeout waiting for CHALLENGE";
            return result;
        }

        const std::string challenge(packet.begin(), packet.end());
        if (challenge != "CHALLENGE")
        {
            result.error = "Unexpected server response";
            return result;
        }

        const std::string auth = "AUTH|" + config.login + "|" + config.password;
        if (!SendMaybeRelay(transport, config, std::vector<std::uint8_t>(auth.begin(), auth.end())))
        {
            result.error = "Failed to send AUTH";
            return result;
        }

        packet.clear();
        if (!transport.ReceiveFrom(packet, peerHost, peerPort, timeoutMs))
        {
            result.error = "Timeout waiting for AUTH result";
            return result;
        }

        const std::string response(packet.begin(), packet.end());
        const auto parts = Split(response, '|');
        if (parts.empty())
        {
            result.error = "Empty AUTH response";
            return result;
        }

        if (parts[0] == "AUTH_FAIL")
        {
            result.error = (parts.size() > 1) ? parts[1] : "Authentication failed";
            return result;
        }

        if (parts[0] != "AUTH_OK")
        {
            result.error = "Invalid AUTH_OK format";
            return result;
        }

        if (parts.size() != 3 && parts.size() != 5)
        {
            result.error = "Invalid AUTH_OK format";
            return result;
        }

        try
        {
            result.sessionId = static_cast<std::uint64_t>(std::stoull(parts[1]));
        }
        catch (...)
        {
            result.error = "Invalid session id";
            return result;
        }

        if (!FromHex(parts[2], result.dataKey))
        {
            result.error = "Invalid key encoding";
            return result;
        }

        if (parts.size() == 5)
        {
            result.virtualIp = parts[3];
            result.virtualSubnet = parts[4];
        }

        result.peerHost = config.host;
        result.peerPort = config.port;
        result.ok = true;
        return result;
    }

    bool SendRelayPacket(
        network::UdpTransport &transport,
        const std::string &coordinatorHost,
        std::uint16_t coordinatorPort,
        const std::string &targetHost,
        std::uint16_t targetPort,
        const std::vector<std::uint8_t> &payload)
    {
        if (targetHost.empty() || targetPort == 0)
        {
            return false;
        }

        return transport.SendTo(
            coordinatorHost,
            coordinatorPort,
            BuildRelayPayload(targetHost, targetPort, payload));
    }

} // namespace linkora::app
