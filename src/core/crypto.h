#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace linkora::core
{

    struct EncryptedPacket
    {
        std::vector<std::uint8_t> nonce;
        std::vector<std::uint8_t> ciphertext;
        std::vector<std::uint8_t> tag;
    };

    class Aes256Gcm
    {
    public:
        static std::vector<std::uint8_t> DeriveKeyPbkdf2(
            const std::string &password,
            const std::vector<std::uint8_t> &salt,
            int iterations);

        static bool Encrypt(
            const std::vector<std::uint8_t> &key,
            const std::vector<std::uint8_t> &plaintext,
            EncryptedPacket &outPacket);

        static bool Decrypt(
            const std::vector<std::uint8_t> &key,
            const EncryptedPacket &packet,
            std::vector<std::uint8_t> &outPlaintext);
    };

} // namespace linkora::core
