#include <iostream>
#include <string>

#include "core/password.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: linkora_hash <password> [iterations]\n";
        return 1;
    }

    const std::string password = argv[1];
    int iterations = 120000;
    if (argc > 2)
    {
        try
        {
            iterations = std::stoi(argv[2]);
        }
        catch (...)
        {
            std::cerr << "Invalid iterations value\n";
            return 1;
        }
    }

    std::string error;
    const std::string hash = linkora::core::BuildPbkdf2Hash(password, iterations, error);
    if (hash.empty())
    {
        std::cerr << "Hash generation failed: " << error << '\n';
        return 1;
    }

    std::cout << hash << '\n';
    return 0;
}
