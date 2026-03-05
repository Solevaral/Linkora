#include "network/frame.h"

#include <cstddef>

#include "core/crypto.h"

namespace linkora::network
{
    namespace
    {
        constexpr std::uint8_t kFrameVersion = 1;
        constexpr std::size_t kFixedHeaderSize = 1 + 8 + 8 + 1 + 1 + 2;

        void AppendU16(std::vector<std::uint8_t> &out, std::uint16_t value)
        {
            out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
            out.push_back(static_cast<std::uint8_t>(value & 0xFF));
        }

        void AppendU64(std::vector<std::uint8_t> &out, std::uint64_t value)
        {
            for (int i = 7; i >= 0; --i)
            {
                out.push_back(static_cast<std::uint8_t>((value >> (8 * i)) & 0xFF));
            }
        }

        bool ReadU16(const std::vector<std::uint8_t> &in, std::size_t pos, std::uint16_t &value)
        {
            if (pos + 2 > in.size())
            {
                return false;
            }
            value = (static_cast<std::uint16_t>(in[pos]) << 8) |
                    static_cast<std::uint16_t>(in[pos + 1]);
            return true;
        }

        bool ReadU64(const std::vector<std::uint8_t> &in, std::size_t pos, std::uint64_t &value)
        {
            if (pos + 8 > in.size())
            {
                return false;
            }
            value = 0;
            for (int i = 0; i < 8; ++i)
            {
                value = (value << 8) | in[pos + i];
            }
            return true;
        }
    } // namespace

    bool BuildEncryptedFrame(
        const std::vector<std::uint8_t> &key,
        const WireHeader &header,
        const std::vector<std::uint8_t> &plaintext,
        std::vector<std::uint8_t> &outFrame)
    {
        core::EncryptedPacket encrypted;
        if (!core::Aes256Gcm::Encrypt(key, plaintext, encrypted))
        {
            return false;
        }

        if (encrypted.nonce.size() > 255 || encrypted.tag.size() > 255 ||
            encrypted.ciphertext.size() > 0xFFFF)
        {
            return false;
        }

        outFrame.clear();
        outFrame.reserve(kFixedHeaderSize + encrypted.nonce.size() + encrypted.tag.size() + encrypted.ciphertext.size());
        outFrame.push_back(kFrameVersion);
        AppendU64(outFrame, header.sessionId);
        AppendU64(outFrame, header.counter);
        outFrame.push_back(static_cast<std::uint8_t>(encrypted.nonce.size()));
        outFrame.push_back(static_cast<std::uint8_t>(encrypted.tag.size()));
        AppendU16(outFrame, static_cast<std::uint16_t>(encrypted.ciphertext.size()));
        outFrame.insert(outFrame.end(), encrypted.nonce.begin(), encrypted.nonce.end());
        outFrame.insert(outFrame.end(), encrypted.tag.begin(), encrypted.tag.end());
        outFrame.insert(outFrame.end(), encrypted.ciphertext.begin(), encrypted.ciphertext.end());
        return true;
    }

    bool ParseEncryptedFrame(
        const std::vector<std::uint8_t> &key,
        const std::vector<std::uint8_t> &frame,
        WireHeader &outHeader,
        std::vector<std::uint8_t> &outPlaintext)
    {
        if (frame.size() < kFixedHeaderSize || frame[0] != kFrameVersion)
        {
            return false;
        }

        std::uint64_t sessionId = 0;
        std::uint64_t counter = 0;
        if (!ReadU64(frame, 1, sessionId) || !ReadU64(frame, 9, counter))
        {
            return false;
        }

        const std::size_t nonceSize = frame[17];
        const std::size_t tagSize = frame[18];
        std::uint16_t ciphertextSize = 0;
        if (!ReadU16(frame, 19, ciphertextSize))
        {
            return false;
        }

        const std::size_t expected = kFixedHeaderSize + nonceSize + tagSize + ciphertextSize;
        if (expected != frame.size())
        {
            return false;
        }

        core::EncryptedPacket encrypted;
        std::size_t pos = kFixedHeaderSize;
        encrypted.nonce.assign(frame.begin() + static_cast<std::ptrdiff_t>(pos), frame.begin() + static_cast<std::ptrdiff_t>(pos + nonceSize));
        pos += nonceSize;
        encrypted.tag.assign(frame.begin() + static_cast<std::ptrdiff_t>(pos), frame.begin() + static_cast<std::ptrdiff_t>(pos + tagSize));
        pos += tagSize;
        encrypted.ciphertext.assign(frame.begin() + static_cast<std::ptrdiff_t>(pos), frame.end());

        if (!core::Aes256Gcm::Decrypt(key, encrypted, outPlaintext))
        {
            return false;
        }

        outHeader.version = kFrameVersion;
        outHeader.sessionId = sessionId;
        outHeader.counter = counter;
        return true;
    }

} // namespace linkora::network
