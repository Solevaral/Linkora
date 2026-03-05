#include "core/tunnel.h"

namespace linkora::core
{

    class LinuxTun final : public ITunnel
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

        void Close() override {}
    };

} // namespace linkora::core
