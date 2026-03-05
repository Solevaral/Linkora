#include <iostream>

#include "network/transport.h"

int main()
{
    linkora::network::UdpTransport transport;
    if (!transport.Bind("0.0.0.0", 0))
    {
        std::cerr << "Failed to initialize client transport" << '\n';
        return 1;
    }

    std::cout << "Linkora client skeleton is running.\n";
    transport.Close();
    return 0;
}
