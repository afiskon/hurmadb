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
        void writeEOL();
        void read(char* buff, size_t buffsize);
        size_t readLine(char* buff, size_t buffsize);

        Socket(Socket const &) = delete;
        void operator=(Socket const &) = delete;
    private:
        int _fd;
};

