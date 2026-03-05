#pragma once

#include <string>

namespace linkora::core
{

    bool VerifyPasswordHash(
        const std::string &password,
        const std::string &storedHash,
        std::string &error);

    std::string BuildPbkdf2Hash(
        const std::string &password,
        int iterations,
        std::string &error);

} // namespace linkora::core
