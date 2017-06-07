/* vim: set ai et ts=4 sw=4: */
#pragma once

#include <string>
#include <unistd.h>
#include <vector>

class Socket {
public:
    Socket(int sock);
    ~Socket();
    void write(const char* buff, size_t buffsize);
    void write(const std::string& buff);
    void read(char* buff, size_t buffsize);
    size_t readLine(char* buff, size_t buffsize);

    Socket(Socket const&) = delete;
    void operator=(Socket const&) = delete;

private:
    static const int _MAX_BUFFER_SIZE = 1024;

    void _bufferedRead();

    int _fd;
    char _buffer[_MAX_BUFFER_SIZE];
    size_t _bufferSize;
};
