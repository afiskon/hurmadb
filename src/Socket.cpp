/* vim: set ai et ts=4 sw=4: */

#include <Socket.h>
#include <algorithm>
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

void Socket::read(char* buff, size_t buffsize) {
    while(buffsize > 0) {
        ssize_t res = ::read(_fd, buff, buffsize);
        if(res == 0) {
            throw std::runtime_error("Socket::read() - client closed connection");
        } else if(res < 0) {
            if(errno == EINTR)
                continue;
            throw std::runtime_error("Socket::read() - read() failed");
        }

        buff += res;
        buffsize -= res;
    }
}

/*
 * Reads a line terminated by \r\n. Returns length of a received string.
 *
 * Current implementation reads bytes one by one. It's probably not the
 * best approach, but it's simple and doesn't require a temporary buffer
 * which should be somehow shifted all the time. So until this method
 * becomes a bottleneck for someone it's good enough.
 */
size_t Socket::readLine(char* buff, size_t buffsize) {
    size_t currsize = 0;
    // TODO: optimize, figure out how many bytes are available using ioctl
    while(currsize < buffsize) {
        ssize_t res = ::read(_fd, buff + currsize, 1);
        if(res <= 0) {
            if(errno == EINTR)
                continue;

            if(res == 0) /* keep this! */
                throw std::runtime_error("Socket::read() - client closed connection");

            std::ostringstream oss;
            oss << "Socket::readLine() - read() failed, res = " << res << ", errno = " << errno;
            throw std::runtime_error(oss.str());
        }

        currsize++;

        if((currsize >= 2) && (buff[currsize - 1] == '\n') &&
           ((buff[currsize - 2] == '\r') || (buff[currsize - 2] == '\n' /* see [1] */))) {
            /* [1] - it's for debugging purpose only, in case developer decided to use `nc` */
            buff[currsize - 2] = '\0';
            return currsize - 2;
        }
    }

    throw std::runtime_error("Socket::readLine() - received string doesn't fit "
                             "into buff, probably it's a garbage");
}
