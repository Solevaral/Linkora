#include "core/tunnel.h"

#include <memory>

namespace linkora::core
{

    class WindowsTun final : public ITunnel
    {
    public:
        bool Open(const char *interfaceName) override
        {
            (void)interfaceName;
            return false;
        }

        int Read(std::vector<std::uint8_t> &outPacket) override
        {
            (void)outPacket;
            return -1;
        }

        int Write(const std::vector<std::uint8_t> &packet) override
        {
            (void)packet;
            return -1;
        }

        const std::string &DeviceName() const override { return deviceName_; }

        void Close() override {}

    private:
        std::string deviceName_ = "wintun0";
    };

    std::unique_ptr<ITunnel> CreatePlatformTunnel()
    {
        return std::make_unique<WindowsTun>();
    }

    bool CreateAndBringUpTunnel(
        ITunnel &tunnel,
        const char *requestedName,
        int mtu,
        std::string &error)
    {
        (void)requestedName;
        (void)mtu;
        if (!tunnel.Open("wintun0"))
        {
            error = "Windows tunnel backend is not implemented yet.";
            return false;
        }
        return true;
    }

} // namespace linkora::core
