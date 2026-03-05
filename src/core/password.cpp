#include "core/password.h"

#include <openssl/crypto.h>
#include <openssl/rand.h>

#include <cstdint>
#include <sstream>
#include <vector>

#include "core/crypto.h"

namespace linkora::core
{
    namespace
    {
        constexpr std::size_t kSaltSize = 16;

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

        bool ConstantTimeEquals(
            const std::vector<std::uint8_t> &lhs,
            const std::vector<std::uint8_t> &rhs)
        {
            if (lhs.size() != rhs.size())
            {
                return false;
            }
            return CRYPTO_memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
        }

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
    } // namespace

    bool VerifyPasswordHash(
        const std::string &password,
        const std::string &storedHash,
        std::string &error)
    {
        const auto parts = Split(storedHash, '$');
        if (parts.size() != 4 || parts[0] != "pbkdf2")
        {
            error = "Unsupported password hash format. Expected pbkdf2$iterations$salt$hash";
            return false;
        }

        int iterations = 0;
        try
        {
            iterations = std::stoi(parts[1]);
        }
        catch (...)
        {
            error = "Invalid iteration value in password hash";
            return false;
        }

        std::vector<std::uint8_t> salt;
        std::vector<std::uint8_t> expectedHash;
        if (!FromHex(parts[2], salt) || !FromHex(parts[3], expectedHash))
        {
            error = "Invalid hex in password hash";
            return false;
        }

        const auto candidate = Aes256Gcm::DeriveKeyPbkdf2(password, salt, iterations);
        if (candidate.empty())
        {
            error = "PBKDF2 failed";
            return false;
        }

        if (!ConstantTimeEquals(candidate, expectedHash))
        {
            error = "Wrong password";
            return false;
        }

        return true;
    }

    std::string BuildPbkdf2Hash(
        const std::string &password,
        int iterations,
        std::string &error)
    {
        if (iterations <= 0)
        {
            error = "Iterations must be positive";
            return {};
        }

        std::vector<std::uint8_t> salt(kSaltSize);
        if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1)
        {
            error = "Failed to generate random salt";
            return {};
        }

        const auto key = Aes256Gcm::DeriveKeyPbkdf2(password, salt, iterations);
        if (key.empty())
        {
            error = "PBKDF2 failed";
            return {};
        }

        return "pbkdf2$" + std::to_string(iterations) + "$" + ToHex(salt) + "$" + ToHex(key);
    }

} // namespace linkora::core
