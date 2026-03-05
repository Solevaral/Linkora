#include "network/route_manager.h"

#include <cstdlib>

namespace linkora::network
{

    bool RouteManager::Setup(const std::string &subnetCidr, const std::string &interfaceName, std::string &error)
    {
        subnet_ = subnetCidr;
        interface_ = interfaceName;

        const std::string cmd = "ip route replace " + subnet_ + " dev " + interface_;
        const int rc = std::system(cmd.c_str());
        if (rc != 0)
        {
            error = "Failed to configure route. Try running as root/CAP_NET_ADMIN";
            return false;
        }

        active_ = true;
        return true;
    }

    void RouteManager::Cleanup()
    {
        if (!active_)
        {
            return;
        }

        const std::string cmd = "ip route del " + subnet_ + " dev " + interface_ + " >/dev/null 2>&1";
        (void)std::system(cmd.c_str());
        active_ = false;
    }

} // namespace linkora::network
