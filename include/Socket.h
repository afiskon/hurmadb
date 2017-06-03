/* vim: set ai et ts=4 sw=4: */
#pragma once

#include <string>
#include <unistd.h>

class Socket {
public:
    Socket(int sock);
    ~Socket();
    void write(const char* buff, size_t buffsize);
    void write(const std::string& buff);
    void read(char* buff, const size_t buffsize);
    size_t readLine(char* buff, const size_t buffsize);

    Socket(Socket const&) = delete;
    void operator=(Socket const&) = delete;

    const size_t MAX_BUFFER_SIZE = 1024;

private:
    void bufferedRead();

    std::string _buffer;
    int _fd;
};
