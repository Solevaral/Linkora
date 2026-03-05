#include "network/transport.h"

#include <cstring>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace linkora::network
{
    namespace
    {
        void CloseFd(int &fd)
        {
            if (fd < 0)
            {
                return;
            }
#if defined(_WIN32)
            closesocket(fd);
#else
            close(fd);
#endif
            fd = -1;
        }
    } // namespace

    bool UdpTransport::Bind(const std::string &host, std::uint16_t port)
    {
        Close();

#if defined(_WIN32)
        if (!winsockInitialized_)
        {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            {
                return false;
            }
            winsockInitialized_ = true;
        }
#endif

        socketFd_ = static_cast<int>(socket(AF_INET, SOCK_DGRAM, 0));
        if (socketFd_ < 0)
        {
            return false;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (host.empty() || host == "0.0.0.0")
        {
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
        {
            Close();
            return false;
        }

        if (bind(socketFd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0)
        {
            Close();
            return false;
        }

        return true;
    }

    bool UdpTransport::SendTo(
        const std::string &host,
        std::uint16_t port,
        const std::vector<std::uint8_t> &payload)
    {
        if (socketFd_ < 0)
        {
            return false;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
        {
            return false;
        }

        const auto *data = payload.empty() ? reinterpret_cast<const std::uint8_t *>("") : payload.data();
        const int sent = static_cast<int>(sendto(
            socketFd_,
            reinterpret_cast<const char *>(data),
            static_cast<int>(payload.size()),
            0,
            reinterpret_cast<sockaddr *>(&addr),
            sizeof(addr)));

        return sent == static_cast<int>(payload.size());
    }

    bool UdpTransport::ReceiveFrom(
        std::vector<std::uint8_t> &payload,
        std::string &peerHost,
        std::uint16_t &peerPort,
        int timeoutMs)
    {
        if (socketFd_ < 0)
        {
            return false;
        }

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(socketFd_, &rfds);

        timeval tv{};
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;

        const int selected = select(socketFd_ + 1, &rfds, nullptr, nullptr, &tv);
        if (selected <= 0)
        {
            return false;
        }

        sockaddr_in from{};
        socklen_t fromLen = sizeof(from);
        std::vector<std::uint8_t> buf(2048);
        const int readBytes = static_cast<int>(recvfrom(
            socketFd_,
            reinterpret_cast<char *>(buf.data()),
            static_cast<int>(buf.size()),
            0,
            reinterpret_cast<sockaddr *>(&from),
            &fromLen));
        if (readBytes <= 0)
        {
            return false;
        }

        buf.resize(static_cast<std::size_t>(readBytes));
        payload.swap(buf);

        char ipBuffer[INET_ADDRSTRLEN] = {0};
        if (!inet_ntop(AF_INET, &from.sin_addr, ipBuffer, sizeof(ipBuffer)))
        {
            return false;
        }
        peerHost = ipBuffer;
        peerPort = ntohs(from.sin_port);
        return true;
    }

    bool UdpTransport::IsOpen() const
    {
        return socketFd_ >= 0;
    }

    void UdpTransport::Close()
    {
        CloseFd(socketFd_);
#if defined(_WIN32)
        if (winsockInitialized_)
        {
            WSACleanup();
            winsockInitialized_ = false;
        }
#endif
    }

} // namespace linkora::network
