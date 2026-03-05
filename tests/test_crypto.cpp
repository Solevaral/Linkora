#include <cstdint>
#include <iostream>
#include <vector>

#include "core/crypto.h"

int main()
{
    const std::vector<std::uint8_t> salt = {1, 2, 3, 4, 5, 6, 7, 8};
    const auto key = linkora::core::Aes256Gcm::DeriveKeyPbkdf2("demo-password", salt, 120000);
    if (key.size() != 32)
    {
        std::cerr << "Key derivation failed\n";
        return 1;
    }

    const std::vector<std::uint8_t> plaintext = {'L', 'i', 'n', 'k', 'o', 'r', 'a'};
    linkora::core::EncryptedPacket packet;
    if (!linkora::core::Aes256Gcm::Encrypt(key, plaintext, packet))
    {
        std::cerr << "Encryption failed\n";
        return 1;
    }

    std::vector<std::uint8_t> decrypted;
    if (!linkora::core::Aes256Gcm::Decrypt(key, packet, decrypted))
    {
        std::cerr << "Decryption failed\n";
        return 1;
    }

    if (decrypted != plaintext)
    {
        std::cerr << "Roundtrip mismatch\n";
        return 1;
    }

    return 0;
}
