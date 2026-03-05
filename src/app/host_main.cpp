#include <iostream>

#include "network/transport.h"

int main()
{
    linkora::network::UdpTransport transport;
    if (!transport.Bind("0.0.0.0", 38123))
    {
        std::cerr << "Failed to bind host transport" << '\n';
        return 1;
    }

    std::cout << "Linkora host skeleton is running.\n";
    transport.Close();
    return 0;
}
