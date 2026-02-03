#include "stomp_client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <string>

namespace py = pybind11;

StompClient::StompClient()
    : host("127.0.0.1"), username("omer"), password("omer"), port(61613)
{
}

StompClient::StompClient(const std::string& host, int port, const std::string& username, const std::string& password)
    : host(host), username(username), password(password), port(port)
{
}

StompClient::~StompClient() noexcept
{
    try
    {
        disconnect();
    }
    catch (...)
    {
    }
}

std::string StompClient::receiveFrame()
{
    std::string response;
    char buffer[4096];

    while (true)
    {
        ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received < 0)
        {
            throw std::runtime_error("Failed to receive response");
        }
        if (bytes_received == 0)
        {
            throw std::runtime_error("Connection closed");
        }

        buffer[bytes_received] = '\0';
        response.append(buffer, bytes_received);

        if (response.find('\0') != std::string::npos) 
        {
            break;
        }
    }

    return response;
}

void StompClient::sendFrame(const std::string& frame)
{
    size_t total_sent = 0;
    while (total_sent < frame.size())
    {
        ssize_t bytes_sent = ::send(sock, frame.data() + total_sent, frame.size() - total_sent, 0);
        if (bytes_sent < 0)
        {
            throw std::runtime_error("Failed to send frame");
        }
        if (bytes_sent == 0)
        {
            throw std::runtime_error("Socket closed while sending");
        }
        total_sent += static_cast<size_t>(bytes_sent);
    }
}

void StompClient::createSocket()
{
    if (sock != -1)
    {
        ::close(sock);
        sock = -1;
        connected = false;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        throw std::runtime_error("Invalid Address");
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0)
    {
        ::close(sock);
        sock = -1;
        throw std::runtime_error("Invalid server address");
    }

    if (::connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        ::close(sock);
        sock = -1;
        throw std::runtime_error("Socket connection failed");
    }
}

void StompClient::connect()
{
    if (connected)
    {
        return;
    }

    createSocket();

    std::string connectFrame =
        "CONNECT\n"
        "accept-version:1.2\n"
        "host:" + host + "\n"
        "login:" + username + "\n"
        "passcode:" + password + "\n"
        "heart-beat:0,0\n"
        "\n";

    sendFrame(connectFrame + '\0');

    std::string response = receiveFrame();
    if (response.find("CONNECTED") == std::string::npos)
    {
        throw std::runtime_error("STOMP connection failed. Response: " + response);
    }

    connected = true;
}

void StompClient::send(const std::string& mnemonic, const py::object& value)
{
    if (!connected)
    {
        throw std::runtime_error("Not connected to the server");
    }

    std::string value_str = py::str(value);
    std::string body = mnemonic + "=" + value_str;

    std::string frame =
        "SEND\n"
        "destination:/queue/telemetry\n"
        "content-type:text/plain\n"
        "content-length:" + std::to_string(body.size()) + "\n"
        "\n" + body;

    sendFrame(frame + '\0');
}

void StompClient::disconnect()
{
    if (!connected)
    {
        if (sock != -1)
        {
            ::close(sock);
            sock = -1;
        }
        return;
    }

    try
    {
        std::string frame = "DISCONNECT\n\n";
        sendFrame(frame + '\0');
    }
    catch (const std::exception&)
    {
    }

    if (sock != -1)
    {
        ::close(sock);
        sock = -1;
    }

    connected = false;
}




PYBIND11_MODULE(stomp_client, m) {
    py::class_<StompClient>(m, "StompClient")
        .def(py::init<>())
        .def(py::init<const std::string&, int, const std::string&, const std::string&>())
        .def("connect", &StompClient::connect)
        .def("send", &StompClient::send)
        .def("disconnect", &StompClient::disconnect);
}
