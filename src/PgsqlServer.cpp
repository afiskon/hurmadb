/* vim: set ai et ts=4 sw=4: */

#include <TcpServer.h>
#include <PgsqlServer.h>
#include <RegexCache.h>
#include <Socket.h>
#include <arpa/inet.h>
#include <atomic>
#include <defer.h>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pcre.h>
#include <pthread.h>
#include <signal.h>
#include <stdexcept>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <tuple>
#include <unistd.h>
#include <utility>
#include <vector>
#include <algorithm>
#include <iterator>

#include "parser.h"
#include "scanner.h"


/*
 **********************************************************************
 * PgsqlWorker
 **********************************************************************
 */

#define MAX_BODY_SIZE 1024 * 1024 /* 1 Mb should probably be enough */

int sock;

struct PgsqlWorkerThreadProcArg {
    int socket;
    std::atomic_int* workersCounter;
};

static void* PgsqlWorkerThreadProc(void* rawArg) {
    PgsqlWorkerThreadProcArg* arg = (PgsqlWorkerThreadProcArg*)rawArg;
    std::atomic_int& workersCounter = *arg->workersCounter;

    defer(workersCounter--); // already incremented in the parent process

    Socket socket(arg->socket);
    PgsqlWorker worker(socket);
    delete arg;

    /* No other thread is going to join() this one */
    pthread_detach(pthread_self());

    worker.run();

    pthread_exit(nullptr);
}

PgsqlWorker::PgsqlWorker(Socket& socket)
  : _socket(socket){
}

size_t PgsqlWorker::getMessageSize(){
    char char_size[4];
    _socket.read(char_size, sizeof(char_size));

    uint32_t size = ntohl(*(uint32_t*)&char_size);
    size -= sizeof(char_size);

    return (size_t) size;
}

void PgsqlWorker::sendParameter(const char* field, const char * parameter, const char* type) {
    uint32_t message_size = htonl(
                                    strlen(type) + 
                                    sizeof(uint32_t) + 
                                    strlen(field) + 
                                    delim_len + 
                                    strlen(parameter)
                                );

    _socket.write(type, strlen(type));
    _socket.write((char*) &message_size, sizeof(message_size));
    _socket.write(field, strlen(field));
    _socket.write((char*) delimeter, delim_len);
    _socket.write(parameter, strlen(parameter));
}

void PgsqlWorker::sendServerConfiguration(){
        _socket.write((char*) "R", 1);

        //Not meaning space
        unsigned char space[] = {0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00};

        _socket.write((char*) space,sizeof(space));

        sendParameter("application_name", "", "S");

        _socket.write((char*) delimeter, delim_len);

        sendParameter("client_encoding", "UTF8", "S");

        _socket.write((char*) delimeter, delim_len);

        sendParameter("DateStyle", "ISO, MDY", "S");

        _socket.write((char*) delimeter, delim_len);

        sendParameter("integer_datetimes", "on", "S");

        _socket.write((char*) delimeter, delim_len);

        sendParameter("IntervalStyle", "postgres", "S");

        _socket.write((char*) delimeter, delim_len);

        sendParameter("is_superuser", "on", "S");

        _socket.write((char*) delimeter, delim_len);

        sendParameter("server_encoding", "UTF8", "S");
        
        _socket.write((char*) delimeter, delim_len);

        sendParameter("server_version", "9.6.5", "S");

        _socket.write((char*) delimeter, delim_len);

        sendParameter("session_authorization", "postgres", "S");

        _socket.write((char*) delimeter, delim_len);

        sendParameter("standard_conforming_strings", "on", "S");

        _socket.write((char*) delimeter, delim_len);

        sendParameter("TimeZone", "W-SU", "S");

        _socket.write((char*) delimeter, delim_len);

        _socket.write((char*) "K", 1);
        
        //Not meaning end of configuration message
        unsigned char end[] = { 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x49, 0x4A, 0x01, 0x08, 0x39, 0xFF, 0x5A, 0x00, 0x00, 0x00, 0x05, 0x49};
    
        _socket.write((char*) end, sizeof(end));
}

void PgsqlWorker::run() {
    size_t size;
    char key[1];
    char* received_message;

    size = getMessageSize();
    received_message = new char[size];
    _socket.read(received_message, size);

    if(received_message[size-1] == '/') {
        
        _socket.write("N", 1);

        size = getMessageSize();
        received_message = new char[size];
        _socket.read(received_message, size);

        //Not meaning end 
        unsigned char answer[13]={0x52, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x05, 0xDE, 0x35, 0xF9, 0xF9};

        _socket.write((char*) answer, 13);

        _socket.read(key, 1);

        size = getMessageSize();
        received_message = new char[size];
        _socket.read(received_message, size);
        
        sendServerConfiguration();

    }
        delete received_message;

        close(sock);
        exit(1);
}

/*
 **********************************************************************
 * PgsqlServer
 **********************************************************************
 */

PgsqlServer::PgsqlServer(): TcpServer(&PgsqlWorkerThreadProc)  
{
}

void my_handler(int s){
           printf("Caught signal %d\n",s);
           close(sock);
           exit(1); 

}

void* PgsqlServer::createWTPArg(int accepted_socket, std::atomic_int* workersCounter){
    sock = accepted_socket;

    auto arg = new(std::nothrow) PgsqlWorkerThreadProcArg();

    if(arg == nullptr) {
        close(accepted_socket);
        throw std::runtime_error("PgsqlServer::createWTPArg() - malloc() call failed");
    }
    arg->socket = accepted_socket;
    arg->workersCounter = workersCounter;
    
    return arg;
}
