/* vim: set ai et ts=4 sw=4: */
#pragma once

#include <string>
#include <unistd.h>
#include <vector>

#define MAX_BUFFER_SIZE 1024

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

private:
    void _bufferedRead();

    char _buffer[MAX_BUFFER_SIZE];
    size_t _bufferSize = 0;
    int _fd;
};
