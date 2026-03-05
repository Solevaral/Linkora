#pragma once

#include <string>

namespace linkora::network
{

    class RouteManager
    {
    public:
        bool Setup(const std::string &subnetCidr, const std::string &interfaceName, std::string &error);
        void Cleanup();

    private:
        std::string subnet_;
        std::string interface_;
        bool active_ = false;
    };

} // namespace linkora::network
