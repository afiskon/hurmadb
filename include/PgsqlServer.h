/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <TcpServer.h>
#include <HttpRequestHandler.h>
#include <Socket.h>
#include <atomic>
#include <pcre.h>
#include <stdexcept>
#include <string>

#define UNUSED(x) (void)(x)

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
    const unsigned char _delimeter[1] = {0x00};
    const unsigned char _range_delim[1] = {0x20};
    const int _delim_len = 1;

    const char* READY_FOR_QUERY_KEY = "Z";   

    const char TRANZACTION_BLOCK_KEY[1] = {'T'}; 



    const char* EMPTY_QUERY_RESPONSE_KEY = "I";

    const char* ERROR_RESPONSE_KEY = "E";

    const char* NOTICE_RESPONSE_KEY = "N";

    const char ROW_DESCRIPTION_KEY[1] = {'T'}; 

    enum _AUTH_REQ {
        AUTH_REQ_OK = 0x30,   /* User is authenticated  */
        AUTH_REQ_KRB4 = 0x31,   /* Kerberos V4. Not supported any more. */
        AUTH_REQ_KRB5 = 0x32,   /* Kerberos V5. Not supported any more. */
        AUTH_REQ_PASSWORD = 0x33,   /* Password */
        AUTH_REQ_CRYPT = 0x34,   /* crypt password. Not supported any more. */
        AUTH_REQ_MD5 = 0x35,   /* md5 password */
        AUTH_REQ_SCM_CREDS = 0x36,   /* transfer SCM credentials */
        AUTH_REQ_GSS = 0x37,   /* GSSAPI without wrap() */
        AUTH_REQ_GSS_CONT = 0x38,   /* Continue GSS exchanges */
        AUTH_REQ_SSPI = 0x39   /* SSPI negotiate without wrap() */
    };

    const char COMMAND_KEY [1]= {'C'}; 

    const char RESPONSE_MESSAGE_KEY [1]= {'R'};  



    void writeCodeAnswer(const char* command, const char * status, const char * response_type);
    void sendSelectQueryResult(int num_of_columns, int num_of_rows);
    void sendInsertQueryResult(int from, int num_of_insertions);
    void sendDeleteQueryResult(int num_of_rows);
    void sendUpdateQueryResult(int num_of_rows);
    size_t getMessageSize();
    void sendServerConfiguration();
    char* readMessage();
    void sendParameter(const char* field, const char * parameter, const char* type);
    void writeSizeOfBlock(size_t szie);
};

class PgsqlServer : public TcpServer {
public:
    PgsqlServer(std::atomic_bool* terminate_flag);
    ~PgsqlServer();
    virtual void* createWorkerThreadProcArg(int accepted_socket, std::atomic_int* workersCounter);
    virtual void addHandler(HttpMethod method, const char* regexpStr, HttpRequestHandler handler) {
        UNUSED(method);
        UNUSED(regexpStr);
        UNUSED(handler);
    };
  
    PgsqlServer(PgsqlServer const&) = delete;
    void operator=(PgsqlServer const&) = delete;
};