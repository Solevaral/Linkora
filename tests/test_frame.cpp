#include <cstdint>
#include <iostream>
#include <vector>

#include "network/frame.h"

int main()
{
    const std::vector<std::uint8_t> key = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};

    linkora::network::WireHeader hdr;
    hdr.sessionId = 42;
    hdr.counter = 7;

    const std::vector<std::uint8_t> plaintext = {'h', 'e', 'l', 'l', 'o'};
    std::vector<std::uint8_t> frame;
    if (!linkora::network::BuildEncryptedFrame(key, hdr, plaintext, frame))
    {
        std::cerr << "BuildEncryptedFrame failed\n";
        return 1;
    }

    linkora::network::WireHeader outHdr;
    std::vector<std::uint8_t> outPlain;
    if (!linkora::network::ParseEncryptedFrame(key, frame, outHdr, outPlain))
    {
        std::cerr << "ParseEncryptedFrame failed\n";
        return 1;
    }

    if (outHdr.sessionId != hdr.sessionId || outHdr.counter != hdr.counter)
    {
        std::cerr << "Header mismatch\n";
        return 1;
    }

    if (outPlain != plaintext)
    {
        std::cerr << "Plaintext mismatch\n";
        return 1;
    }

    frame.back() ^= 0x01;
    if (linkora::network::ParseEncryptedFrame(key, frame, outHdr, outPlain))
    {
        std::cerr << "Tampered frame should not decrypt\n";
        return 1;
    }

    return 0;
}
