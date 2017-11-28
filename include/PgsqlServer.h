/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <TcpServer.h>
#include <Socket.h>
#include <atomic>
#include <pcre.h>
#include <stdexcept>
#include <string>

#define AUTH_REQ_OK         0x30   /* User is authenticated  */
#define AUTH_REQ_KRB4       0x31   /* Kerberos V4. Not supported any more. */
#define AUTH_REQ_KRB5       0x32   /* Kerberos V5. Not supported any more. */
#define AUTH_REQ_PASSWORD   0x33   /* Password */
#define AUTH_REQ_CRYPT      0x34   /* crypt password. Not supported any more. */
#define AUTH_REQ_MD5        0x35   /* md5 password */
#define AUTH_REQ_SCM_CREDS  0x36   /* transfer SCM credentials */
#define AUTH_REQ_GSS        0x37   /* GSSAPI without wrap() */
#define AUTH_REQ_GSS_CONT   0x38   /* Continue GSS exchanges */
#define AUTH_REQ_SSPI       0x39   /* SSPI negotiate without wrap() */
#define AUTH_REQ_SASL       10   /* Begin SASL authentication */
#define AUTH_REQ_SASL_CONT  0x31, 0x31   /* Continue SASL authentication */
#define AUTH_REQ_SASL_FIN   12   /* Final SASL message */


class PgsqlWorkerException : public std::exception {
public:
    explicit PgsqlWorkerException(const char* msg)
      : _msg(msg) {
    }

    virtual const char* what() const throw() {
        return _msg.c_str();
    }

private:
    std::string _msg;
};

class PgsqlWorker {
public:
    PgsqlWorker(Socket& socket);
    void run();

private:
    Socket& _socket;
    const unsigned char delimeter[1] = {0x00};
    const int delim_len = 1;

    size_t getMessageSize();
    void sendServerConfiguration();
    void sendParameter(const char* field, const char * parameter, const char* type);
};

class PgsqlServer : public TcpServer {
public:
    PgsqlServer();
    virtual void* createWTPArg(int accepted_socket, std::atomic_int* workersCounter);

    PgsqlServer(PgsqlServer const&) = delete;
    void operator=(PgsqlServer const&) = delete;
};