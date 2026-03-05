#include "core/tunnel.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdlib>

namespace linkora::core
{

    class LinuxTun final : public ITunnel
    {
    public:
        bool Open(const char *interfaceName) override
        {
            if (fd_ >= 0)
            {
                return true;
            }

            fd_ = open("/dev/net/tun", O_RDWR);
            if (fd_ < 0)
            {
                return false;
            }

            struct ifreq ifr{};
            ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

            if (interfaceName && interfaceName[0] != '\0')
            {
                std::strncpy(ifr.ifr_name, interfaceName, IFNAMSIZ - 1);
            }

            if (ioctl(fd_, TUNSETIFF, &ifr) < 0)
            {
                Close();
                return false;
            }

            deviceName_ = ifr.ifr_name;
            return true;
        }

        int Read(std::vector<std::uint8_t> &outPacket) override
        {
            if (fd_ < 0)
            {
                return -1;
            }
            outPacket.resize(2000);
            const int n = static_cast<int>(read(fd_, outPacket.data(), outPacket.size()));
            if (n <= 0)
            {
                outPacket.clear();
                return -1;
            }
            outPacket.resize(static_cast<std::size_t>(n));
            return n;
        }

        int Write(const std::vector<std::uint8_t> &packet) override
        {
            if (fd_ < 0 || packet.empty())
            {
                return -1;
            }
            return static_cast<int>(write(fd_, packet.data(), packet.size()));
        }

        const std::string &DeviceName() const override { return deviceName_; }

        void Close() override
        {
            if (fd_ >= 0)
            {
                close(fd_);
                fd_ = -1;
            }
            deviceName_.clear();
        }

        ~LinuxTun() override { Close(); }

    private:
        int fd_ = -1;
        std::string deviceName_;
    };

    std::unique_ptr<ITunnel> CreatePlatformTunnel()
    {
        return std::make_unique<LinuxTun>();
    }

    bool CreateAndBringUpTunnel(
        ITunnel &tunnel,
        const char *requestedName,
        int mtu,
        std::string &error)
    {
        if (!tunnel.Open(requestedName))
        {
            error = "Failed to open /dev/net/tun. Root/CAP_NET_ADMIN is required.";
            return false;
        }

        const std::string &dev = tunnel.DeviceName();
        const std::string upCmd = "ip link set dev " + dev + " up mtu " + std::to_string(mtu);
        if (std::system(upCmd.c_str()) != 0)
        {
            error = "Failed to set link up for " + dev;
            return false;
        }

        return true;
    }

} // namespace linkora::core
