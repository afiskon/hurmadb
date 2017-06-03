/* vim: set ai et ts=4 sw=4: */

#include <Socket.h>
#include <algorithm>
#include <errno.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include <cstring>

Socket::Socket(int sock) {
    _fd = sock;
}

Socket::~Socket() {
    close(_fd);
}

void Socket::write(const char* buff, size_t buffsize) {
    while(buffsize > 0) {
        ssize_t res = ::write(_fd, buff, buffsize);
        if(res <= 0) {
            if(errno == EINTR)
                continue;
            throw std::runtime_error("Socket::write() - write() failed");
        }

        buff += res;
        buffsize -= res;
    }
}

void Socket::write(const std::string& buff) {
    write(buff.c_str(), buff.size());
}

void Socket::read(char* buff, const size_t buffsize) {
    while(_buffer.size() < buffsize)
        bufferedRead();

    strncpy(buff, _buffer.c_str(), buffsize);
    buff[buffsize] = '\0';
    _buffer = _buffer.substr(buffsize);
}

/*
 * Reads a line terminated by \r\n. Returns length of a received string.
 */
size_t Socket::readLine(char* buff, const size_t buffsize) {
    while(_buffer.find_first_of('\n', 0) == std::string::npos)
        bufferedRead();

    size_t pos = _buffer.find_first_of('\n', 0);
    if(pos > buffsize)
        throw std::runtime_error("Socket::readLine - line too long");

    strncpy(buff, _buffer.c_str(), pos);
    buff[pos] = '\0';
    _buffer = _buffer.substr(pos + 1);

    if(pos > 0 && buff[pos - 1] == '\r') {
        pos--;
        buff[pos] = '\0';
    }

    return pos;
}

void Socket::bufferedRead() {
    char buff[MAX_BUFFER_SIZE];
    ssize_t res = ::read(_fd, buff, MAX_BUFFER_SIZE);
    buff[res] = '\0';
    if(res <= 0) {
        if(res == 0) /* keep this! */
            throw std::runtime_error("Socket::read() - client closed connection");

        std::ostringstream oss;
        oss << "Socket::readLine() - read() failed, res = " << res << ", errno = " << errno;
        throw std::runtime_error(oss.str());
    }

    if(res + _buffer.length() > MAX_BUFFER_SIZE)
        throw std::runtime_error("Socket::bufferedRead() - received string doesn't fit "
                                 "into buff, probably it's a garbage");

    _buffer += buff;
}
