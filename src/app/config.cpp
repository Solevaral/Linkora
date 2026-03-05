#include "app/config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace linkora::app
{
    namespace
    {
        std::string Trim(const std::string &value)
        {
            const auto begin = value.find_first_not_of(" \t\r\n");
            if (begin == std::string::npos)
            {
                return {};
            }
            const auto end = value.find_last_not_of(" \t\r\n");
            return value.substr(begin, end - begin + 1);
        }

        std::string StripQuotes(const std::string &value)
        {
            if (value.size() >= 2 &&
                ((value.front() == '"' && value.back() == '"') ||
                 (value.front() == '\'' && value.back() == '\'')))
            {
                return value.substr(1, value.size() - 2);
            }
            return value;
        }

        bool ParseSimpleYaml(
            const std::string &path,
            std::unordered_map<std::string, std::string> &out,
            std::string &error)
        {
            std::ifstream input(path);
            if (!input)
            {
                error = "Cannot open config file: " + path;
                return false;
            }

            std::string currentSection;
            std::string line;
            int lineNo = 0;
            while (std::getline(input, line))
            {
                ++lineNo;
                const auto hashPos = line.find('#');
                if (hashPos != std::string::npos)
                {
                    line = line.substr(0, hashPos);
                }

                if (Trim(line).empty())
                {
                    continue;
                }

                const bool sectionLine = !line.empty() && line.find(':') == line.size() - 1 && line.find_first_not_of(' ') == 0;
                if (sectionLine)
                {
                    currentSection = Trim(line.substr(0, line.size() - 1));
                    continue;
                }

                const auto keyPos = line.find(':');
                if (keyPos == std::string::npos)
                {
                    error = "Invalid config line " + std::to_string(lineNo);
                    return false;
                }

                auto key = Trim(line.substr(0, keyPos));
                auto value = StripQuotes(Trim(line.substr(keyPos + 1)));
                if (key.empty())
                {
                    error = "Empty key at line " + std::to_string(lineNo);
                    return false;
                }

                if (!currentSection.empty() && line.find_first_not_of(' ') != 0)
                {
                    key = currentSection + "." + key;
                }

                out[key] = value;
            }

            return true;
        }

        bool ParseU16(const std::string &value, std::uint16_t &out)
        {
            try
            {
                const int parsed = std::stoi(value);
                if (parsed < 0 || parsed > 65535)
                {
                    return false;
                }
                out = static_cast<std::uint16_t>(parsed);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        bool ParseInt(const std::string &value, int &out)
        {
            try
            {
                out = std::stoi(value);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        std::string DeriveSubnetFromName(const std::string &name)
        {
            std::uint32_t hash = 2166136261u;
            for (unsigned char ch : name)
            {
                const unsigned char normalized = static_cast<unsigned char>(std::tolower(ch));
                hash ^= static_cast<std::uint32_t>(normalized);
                hash *= 16777619u;
            }

            const int octet2 = 16 + static_cast<int>(hash % 128u); // 10.16.0.0 - 10.143.255.0
            int octet3 = static_cast<int>((hash >> 8) & 0xFFu);
            if (octet3 == 0)
            {
                octet3 = 1;
            }
            return "10." + std::to_string(octet2) + "." + std::to_string(octet3) + ".0/24";
        }
    } // namespace

    bool LoadHostConfig(const std::string &path, HostConfig &outConfig, std::string &error)
    {
        std::unordered_map<std::string, std::string> values;
        if (!ParseSimpleYaml(path, values, error))
        {
            return false;
        }

        outConfig.networkName = values["network.name"];
        outConfig.listenHost = values["network.listen_host"];
        if (!ParseU16(values["network.listen_port"], outConfig.listenPort))
        {
            error = "Invalid network.listen_port";
            return false;
        }
        outConfig.coordinatorHost = values["coordinator.host"];
        if (!ParseU16(values["coordinator.port"], outConfig.coordinatorPort))
        {
            error = "Invalid coordinator.port";
            return false;
        }
        outConfig.login = values["auth.login"];
        outConfig.passwordHash = values["auth.password_hash"];
        outConfig.passwordPlain = values["auth.password"];
        outConfig.virtualSubnet = values["vpn.virtual_subnet"];
        if (!values["vpn.mtu"].empty() && !ParseInt(values["vpn.mtu"], outConfig.mtu))
        {
            error = "Invalid vpn.mtu";
            return false;
        }

        if (outConfig.listenHost.empty() || outConfig.login.empty() || outConfig.coordinatorHost.empty())
        {
            error = "Host config missing required fields";
            return false;
        }

        if (outConfig.virtualSubnet.empty() || outConfig.virtualSubnet == "auto")
        {
            const std::string baseName = !outConfig.networkName.empty() ? outConfig.networkName : outConfig.login;
            outConfig.virtualSubnet = DeriveSubnetFromName(baseName);
        }

        if (outConfig.passwordHash.empty() && outConfig.passwordPlain.empty())
        {
            error = "Host config requires auth.password_hash or auth.password";
            return false;
        }

        return true;
    }

    bool LoadClientConfig(const std::string &path, ClientConfig &outConfig, std::string &error)
    {
        std::unordered_map<std::string, std::string> values;
        if (!ParseSimpleYaml(path, values, error))
        {
            return false;
        }

        outConfig.host = values["network.host"];
        if (!ParseU16(values["network.port"], outConfig.port))
        {
            error = "Invalid network.port";
            return false;
        }
        outConfig.login = values["auth.login"];
        outConfig.password = values["auth.password"];
        outConfig.virtualIp = values["vpn.virtual_ip"];
        if (!values["vpn.mtu"].empty() && !ParseInt(values["vpn.mtu"], outConfig.mtu))
        {
            error = "Invalid vpn.mtu";
            return false;
        }

        if (outConfig.host.empty() || outConfig.login.empty() || outConfig.password.empty())
        {
            error = "Client config missing required fields";
            return false;
        }

        return true;
    }

} // namespace linkora::app
