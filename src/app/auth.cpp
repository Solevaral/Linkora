#include "app/auth.h"

#include <openssl/rand.h>

#include <algorithm>
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

        const std::string ok = "AUTH_OK|" + std::to_string(result.sessionId) + "|" + ToHex(result.dataKey);
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

        const std::string hello = "HELLO|" + config.login;
        if (!transport.SendTo(config.host, config.port, std::vector<std::uint8_t>(hello.begin(), hello.end())))
        {
            result.error = "Failed to send HELLO";
            return result;
        }

        std::vector<std::uint8_t> packet;
        std::string peerHost;
        std::uint16_t peerPort = 0;
        if (!transport.ReceiveFrom(packet, peerHost, peerPort, timeoutMs))
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
        if (!transport.SendTo(config.host, config.port, std::vector<std::uint8_t>(auth.begin(), auth.end())))
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

        if (parts.size() != 3 || parts[0] != "AUTH_OK")
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

        result.peerHost = config.host;
        result.peerPort = config.port;
        result.ok = true;
        return result;
    }

} // namespace linkora::app
