#include "network/route_manager.h"

namespace linkora::network
{

    bool RouteManager::Setup(const std::string &subnetCidr, const std::string &interfaceName, std::string &error)
    {
        (void)subnetCidr;
        (void)interfaceName;
        error = "Windows route setup is not implemented yet."
                " Use route add manually for MVP.";
        return false;
    }

    void RouteManager::Cleanup() {}

} // namespace linkora::network
