#pragma once

#include <cstdint>
#include <vector>

namespace linkora::network
{

    struct WireHeader
    {
        std::uint8_t version = 1;
        std::uint64_t sessionId = 0;
        std::uint64_t counter = 0;
    };

    bool BuildEncryptedFrame(
        const std::vector<std::uint8_t> &key,
        const WireHeader &header,
        const std::vector<std::uint8_t> &plaintext,
        std::vector<std::uint8_t> &outFrame);

    bool ParseEncryptedFrame(
        const std::vector<std::uint8_t> &key,
        const std::vector<std::uint8_t> &frame,
        WireHeader &outHeader,
        std::vector<std::uint8_t> &outPlaintext);

} // namespace linkora::network
