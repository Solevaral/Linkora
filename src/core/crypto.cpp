#include "core/crypto.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

namespace linkora::core
{
    namespace
    {
        constexpr int kKeySize = 32;
        constexpr int kNonceSize = 12;
        constexpr int kTagSize = 16;
    } // namespace

    std::vector<std::uint8_t> Aes256Gcm::DeriveKeyPbkdf2(
        const std::string &password,
        const std::vector<std::uint8_t> &salt,
        int iterations)
    {
        std::vector<std::uint8_t> key(kKeySize);
        const int ok = PKCS5_PBKDF2_HMAC(
            password.c_str(),
            static_cast<int>(password.size()),
            salt.data(),
            static_cast<int>(salt.size()),
            iterations,
            EVP_sha256(),
            kKeySize,
            key.data());
        if (ok != 1)
        {
            key.clear();
        }
        return key;
    }

    bool Aes256Gcm::Encrypt(
        const std::vector<std::uint8_t> &key,
        const std::vector<std::uint8_t> &plaintext,
        EncryptedPacket &outPacket)
    {
        if (key.size() != kKeySize)
        {
            return false;
        }

        outPacket.nonce.resize(kNonceSize);
        outPacket.ciphertext.resize(plaintext.size());
        outPacket.tag.resize(kTagSize);

        if (RAND_bytes(outPacket.nonce.data(), kNonceSize) != 1)
        {
            return false;
        }

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx)
        {
            return false;
        }

        int ok = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
        ok &= EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, kNonceSize, nullptr);
        ok &= EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), outPacket.nonce.data());

        int outLen = 0;
        int total = 0;
        if (ok == 1 && !plaintext.empty())
        {
            ok = EVP_EncryptUpdate(
                ctx,
                outPacket.ciphertext.data(),
                &outLen,
                plaintext.data(),
                static_cast<int>(plaintext.size()));
            total = outLen;
        }

        if (ok == 1)
        {
            ok = EVP_EncryptFinal_ex(ctx, outPacket.ciphertext.data() + total, &outLen);
            total += outLen;
        }

        if (ok == 1)
        {
            ok = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, kTagSize, outPacket.tag.data());
        }

        EVP_CIPHER_CTX_free(ctx);

        if (ok != 1)
        {
            return false;
        }

        outPacket.ciphertext.resize(static_cast<std::size_t>(total));
        return true;
    }

    bool Aes256Gcm::Decrypt(
        const std::vector<std::uint8_t> &key,
        const EncryptedPacket &packet,
        std::vector<std::uint8_t> &outPlaintext)
    {
        if (key.size() != kKeySize ||
            packet.nonce.size() != kNonceSize ||
            packet.tag.size() != kTagSize)
        {
            return false;
        }

        outPlaintext.resize(packet.ciphertext.size());

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx)
        {
            return false;
        }

        int ok = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
        ok &= EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, kNonceSize, nullptr);
        ok &= EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), packet.nonce.data());

        int outLen = 0;
        int total = 0;
        if (ok == 1 && !packet.ciphertext.empty())
        {
            ok = EVP_DecryptUpdate(
                ctx,
                outPlaintext.data(),
                &outLen,
                packet.ciphertext.data(),
                static_cast<int>(packet.ciphertext.size()));
            total = outLen;
        }

        if (ok == 1)
        {
            ok = EVP_CIPHER_CTX_ctrl(
                ctx,
                EVP_CTRL_GCM_SET_TAG,
                kTagSize,
                const_cast<std::uint8_t *>(packet.tag.data()));
        }

        if (ok == 1)
        {
            ok = EVP_DecryptFinal_ex(ctx, outPlaintext.data() + total, &outLen);
            total += outLen;
        }

        EVP_CIPHER_CTX_free(ctx);

        if (ok != 1)
        {
            outPlaintext.clear();
            return false;
        }

        outPlaintext.resize(static_cast<std::size_t>(total));
        return true;
    }

} // namespace linkora::core
