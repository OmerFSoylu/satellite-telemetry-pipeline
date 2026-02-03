#pragma once

#include <string>
#include <pybind11/pybind11.h>

namespace py = pybind11;

class StompClient
{
public:
    StompClient();
    StompClient(const std::string& host, int port, const std::string& username, const std::string& password);
    ~StompClient() noexcept;

    void connect();
    void send(const std::string& mnemonic, const py::object& value);
    void disconnect();

private:
    std::string host;
    std::string username;
    std::string password;
    int port;
    int sock = -1;
    bool connected = false;

    void createSocket();
    std::string receiveFrame();
    void sendFrame(const std::string& frame);
};
