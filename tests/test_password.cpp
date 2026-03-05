#include <iostream>
#include <string>

#include "core/password.h"

int main()
{
    std::string error;
    const std::string hash = linkora::core::BuildPbkdf2Hash("dev-password", 120000, error);
    if (hash.empty())
    {
        std::cerr << "Build hash failed: " << error << '\n';
        return 1;
    }

    if (!linkora::core::VerifyPasswordHash("dev-password", hash, error))
    {
        std::cerr << "Verify should pass but failed: " << error << '\n';
        return 1;
    }

    if (linkora::core::VerifyPasswordHash("wrong", hash, error))
    {
        std::cerr << "Verify should fail for wrong password\n";
        return 1;
    }

    return 0;
}
