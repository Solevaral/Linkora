#include "app/port_open.h"

#include <cstdlib>
#include <string>

namespace linkora::app
{
    namespace
    {
        bool RunCmd(const std::string &cmd)
        {
            return std::system(cmd.c_str()) == 0;
        }
    }

    bool EnsureUdpPortOpen(std::uint16_t port, std::string &error)
    {
        const std::string portStr = std::to_string(port);

#if defined(_WIN32)
        const std::string ruleName = "Linkora UDP " + portStr;
        const std::string existsCmd =
            "netsh advfirewall firewall show rule name=\"" + ruleName + "\" >nul 2>&1";
        if (RunCmd(existsCmd))
        {
            return true;
        }

        const std::string addCmd =
            "netsh advfirewall firewall add rule name=\"" + ruleName +
            "\" dir=in action=allow protocol=UDP localport=" + portStr + " >nul 2>&1";
        if (!RunCmd(addCmd))
        {
            error = "Failed to add Windows Firewall rule for UDP port " + portStr +
                    ". Run app as Administrator or open the port manually.";
            return false;
        }
        return true;
#else
        if (RunCmd("command -v ufw >/dev/null 2>&1"))
        {
            const std::string directCmd = "ufw allow " + portStr + "/udp >/dev/null 2>&1";
            if (RunCmd(directCmd))
            {
                return true;
            }

            if (RunCmd("command -v sudo >/dev/null 2>&1"))
            {
                const std::string sudoCmd = "sudo -n ufw allow " + portStr + "/udp >/dev/null 2>&1";
                if (RunCmd(sudoCmd))
                {
                    return true;
                }
            }

            error = "Failed to open UDP port " + portStr +
                    " via ufw. Use sudo privileges or open the port manually.";
            return false;
        }

        if (RunCmd("command -v firewall-cmd >/dev/null 2>&1"))
        {
            const std::string cmd =
                "firewall-cmd --permanent --add-port=" + portStr + "/udp >/dev/null 2>&1 && firewall-cmd --reload >/dev/null 2>&1";
            if (RunCmd(cmd))
            {
                return true;
            }

            if (RunCmd("command -v sudo >/dev/null 2>&1"))
            {
                const std::string sudoCmd =
                    "sudo -n firewall-cmd --permanent --add-port=" + portStr +
                    "/udp >/dev/null 2>&1 && sudo -n firewall-cmd --reload >/dev/null 2>&1";
                if (RunCmd(sudoCmd))
                {
                    return true;
                }
            }

            error = "Failed to open UDP port " + portStr +
                    " via firewall-cmd. Use sudo privileges or open the port manually.";
            return false;
        }

        return true;
#endif
    }
}
