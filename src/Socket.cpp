/* vim: set ai et ts=4 sw=4: */

#include <Socket.h>
#include <algorithm>
#include <cstring>
#include <errno.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

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

/*
 * Reads buffsize bytes. Returns number of bytes read.
 */
void Socket::read(char* buff, size_t buffsize) {
    if(buffsize > MAX_BUFFER_SIZE)
        throw new std::invalid_argument("Socket::read(): buffsize too large");

    while(_bufferSize < buffsize)
        _bufferedRead();

    memcpy(buff, _buffer, buffsize);
    _bufferSize -= buffsize;
    memmove(_buffer, _buffer + buffsize, _bufferSize);
}

/*
 * Reads a line terminated by \r\n or \n. Returns length of a received string.
 */
size_t Socket::readLine(char* buff, size_t buffsize) {
    size_t end = 0;
    do {
        if(end >= buffsize)
            throw std::runtime_error("Socket::readLine - line too long");
        if(end == _bufferSize)
            _bufferedRead();
        else
            end++;
    } while(_buffer[end] != '\n');

    memcpy(buff, _buffer, end);
    buff[end] = '\0';
    _bufferSize -= end + 1;
    memmove(_buffer, _buffer + end + 1, _bufferSize);
    if(end > 0 && buff[end - 1] == '\r') {
        end--;
        buff[end] = '\0';
    }
    return end;
}

void Socket::_bufferedRead() {
    size_t count = MAX_BUFFER_SIZE - _bufferSize;
    ssize_t res;
    do {
        res = ::read(_fd, _buffer + _bufferSize, count);
        if(res <= 0) {
            if(errno == EINTR)
                continue;
            if(res == 0) /* keep this! */
                throw std::runtime_error("Socket::read() - client closed connection");

            throw std::runtime_error("Socket::readLine() - read() failed: " + std::string(strerror(errno)));
        }
    } while(res <= 0 && errno == EINTR);
    _bufferSize += res;
}
