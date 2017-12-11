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
#include <regex>

using namespace std;

/*
 **********************************************************************
 * PgsqlWorker
 **********************************************************************
 */

#define MAX_BODY_SIZE 1024 * 1024 /* 1 Mb should probably be enough */

int sock;
std::atomic_bool* terminate_flag_local;

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
                                    _delim_len + 
                                    strlen(parameter)
                                );

    _socket.write(type, strlen(type));
    _socket.write((char*) &message_size, sizeof(message_size));
    _socket.write(field, strlen(field));
    _socket.write((char*) _delimeter, _delim_len);
    _socket.write(parameter, strlen(parameter));
}

void PgsqlWorker::sendServerConfiguration(){
        _socket.write((char*) RESPONSE_MESSAGE_KEY, sizeof(RESPONSE_MESSAGE_KEY));

        //Space in start of configuration message
        writeSizeOfBlock(sizeof(uint32_t) + _delim_len * 4);

        _socket.write((char*) _delimeter, _delim_len);

        _socket.write((char*) _delimeter, _delim_len);

        _socket.write((char*) _delimeter, _delim_len);

        _socket.write((char*) _delimeter, _delim_len);
        //End of space in start message

        sendParameter("application_name", "", "S");

        _socket.write((char*) _delimeter, _delim_len);

        sendParameter("client_encoding", "UTF8", "S");

        _socket.write((char*) _delimeter, _delim_len);

        sendParameter("DateStyle", "ISO, MDY", "S");

        _socket.write((char*) _delimeter, _delim_len);

        sendParameter("integer_datetimes", "on", "S");

        _socket.write((char*) _delimeter, _delim_len);

        sendParameter("IntervalStyle", "postgres", "S");

        _socket.write((char*) _delimeter, _delim_len);

        sendParameter("is_superuser", "on", "S");

        _socket.write((char*) _delimeter, _delim_len);

        sendParameter("server_encoding", "UTF8", "S");
        
        _socket.write((char*) _delimeter, _delim_len);

        sendParameter("server_version", "9.6.5", "S");

        _socket.write((char*) _delimeter, _delim_len);

        sendParameter("session_authorization", "postgres", "S");

        _socket.write((char*) _delimeter, _delim_len);

        sendParameter("standard_conforming_strings", "on", "S");

        _socket.write((char*) _delimeter, _delim_len);

        sendParameter("TimeZone", "W-SU", "S");

        _socket.write((char*) _delimeter, _delim_len);

        _socket.write((char*) "K");
        
        //Not meaning end of configuration message
        unsigned char end[] = { 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x49, 0x4A, 0x01, 0x08, 0x39, 0xFF, 0x5A, 0x00, 0x00, 0x00, 0x05, 0x49};
    
        _socket.write((char*) end, sizeof(end));
}

void PgsqlWorker::writeCodeAnswer(const char* command, const char* status, const char * response_type){
    uint32_t message_size = htonl(sizeof(uint32_t) + strlen(command) + strlen(status));
    
    _socket.write(COMMAND_KEY, sizeof(COMMAND_KEY)); //Code key
    _socket.write((char*) &message_size, sizeof(message_size));
    _socket.write(command, strlen(command));
    _socket.write((char*) _delimeter, _delim_len);
    _socket.write(status, strlen(status));

    message_size = htonl(sizeof(uint32_t) + strlen(response_type));

    _socket.write((char*) &message_size, sizeof(message_size));
    _socket.write(response_type, strlen(response_type));
}

char* PgsqlWorker::readMessage(){
    size_t size;
    char* received_message;

    size = getMessageSize();
    received_message = new char[size];
    _socket.read(received_message, size);

    return received_message;
}

void PgsqlWorker::sendSelectQueryResult(int num_of_columns, int num_of_rows){
    const char resulted_rows_delimiter[] = {'C'};
    const char* select_command = "SELECT";
    const char* num_of_rows_value = to_string(num_of_rows).c_str();
    const char* num_of_columns_value = to_string(num_of_columns).c_str();     
    const char* anon_column = "?column?";

    _socket.write((char*) ROW_DESCRIPTION_KEY, sizeof(ROW_DESCRIPTION_KEY));

    unsigned char columns[]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00}; 

    writeSizeOfBlock(33);

    _socket.write((char*) _delimeter, _delim_len);

    _socket.write((char*) &num_of_columns_value, strlen(num_of_columns_value));

    _socket.write((char*) anon_column, strlen(anon_column));

    _socket.write((char*) columns, sizeof(columns));

    _socket.write((char*) resulted_rows_delimiter, sizeof(resulted_rows_delimiter)); //Data row delimeter
    
    writeSizeOfBlock(13); //Message size
    
    _socket.write((char*) _delimeter, _delim_len);

    _socket.write((char*) &num_of_rows_value, strlen(num_of_rows_value));
    
    const char* val = "151";

    writeSizeOfBlock(3);

    _socket.write((char*) val, 3);

    /* Code message part */
    _socket.write((char*)COMMAND_KEY, sizeof(COMMAND_KEY));
    writeSizeOfBlock(
            sizeof(uint32_t) +
            strlen(select_command) + 
            _delim_len + 
            strlen(num_of_rows_value) + 
            strlen(READY_FOR_QUERY_KEY)
        );
    _socket.write(select_command, strlen(select_command));
    _socket.write((char*) &_range_delim, _delim_len);
    _socket.write(num_of_rows_value, strlen(num_of_rows_value));
    _socket.write((char*) _delimeter, _delim_len);
    _socket.write((char*) READY_FOR_QUERY_KEY, strlen(READY_FOR_QUERY_KEY));
     writeSizeOfBlock(sizeof(uint32_t) + strlen(TRANZACTION_BLOCK_KEY));
    _socket.write((char*) TRANZACTION_BLOCK_KEY, strlen(TRANZACTION_BLOCK_KEY));   
}

/*
Send to client next message: 
    C...DELETE.from.num_of_insertions.Z...T
where from - is the start index of insertions, num_of_insertions - the number of inserted rows
*/
void PgsqlWorker::sendInsertQueryResult(int from, int num_of_insertions){
    const char* insert_command = "INSERT";
    const char* from_value = to_string(from).c_str();
    const char* num_of_val = to_string(num_of_insertions).c_str();

    _socket.write((char*)COMMAND_KEY, sizeof(COMMAND_KEY));
    writeSizeOfBlock(
            sizeof(uint32_t) +
            strlen(insert_command) + 
            _delim_len + 
            strlen(from_value) + 
            _delim_len + 
            strlen(num_of_val) +
            strlen(READY_FOR_QUERY_KEY)
        );
    _socket.write(insert_command, strlen(insert_command));
    _socket.write((char*) &_range_delim, _delim_len);
    _socket.write(from_value, strlen(from_value));
    _socket.write((char*) &_range_delim, _delim_len);
    _socket.write(num_of_val, strlen(num_of_val));
    _socket.write((char*) _delimeter, _delim_len);
    _socket.write((char*) READY_FOR_QUERY_KEY, strlen(READY_FOR_QUERY_KEY));
     writeSizeOfBlock(sizeof(uint32_t) + sizeof(TRANZACTION_BLOCK_KEY));
    _socket.write((char*) TRANZACTION_BLOCK_KEY, strlen(TRANZACTION_BLOCK_KEY));   
}

/*
Send to client next message: 
    C...DELETE.num_of_rows.Z...T
where num_of_rows is the number of deleted rows
*/
void PgsqlWorker::sendDeleteQueryResult(int num_of_rows){
    const char* delete_command = "DELETE";
    const char* num_of_rows_value = to_string(num_of_rows).c_str();

    _socket.write((char*)COMMAND_KEY, sizeof(COMMAND_KEY));
    writeSizeOfBlock(
            sizeof(uint32_t) +
            strlen(delete_command) + 
            _delim_len + 
            strlen(num_of_rows_value) + 
            strlen(READY_FOR_QUERY_KEY)
        );
    _socket.write(delete_command, strlen(delete_command));
    _socket.write((char*) &_range_delim, _delim_len);
    _socket.write(num_of_rows_value, strlen(num_of_rows_value));
    _socket.write((char*) _delimeter, _delim_len);
    _socket.write((char*) READY_FOR_QUERY_KEY, strlen(READY_FOR_QUERY_KEY));
     writeSizeOfBlock(sizeof(uint32_t) + strlen(TRANZACTION_BLOCK_KEY));
    _socket.write((char*) TRANZACTION_BLOCK_KEY, strlen(TRANZACTION_BLOCK_KEY));   
}
/*
Send to client next message: 
    C...UPDATE.num_of_rows.Z...T
where num_of_rows is the number of updated rows
*/
void PgsqlWorker::sendUpdateQueryResult(int num_of_rows){
    const char* update_command = "UPDATE";
    const char* num_of_rows_value = to_string(num_of_rows).c_str();

    _socket.write((char*)COMMAND_KEY, sizeof(COMMAND_KEY));
    writeSizeOfBlock(
            sizeof(uint32_t) +
            strlen(update_command) + 
            _delim_len + 
            strlen(num_of_rows_value) + 
            strlen(READY_FOR_QUERY_KEY)
        );
    _socket.write(update_command, strlen(update_command));
    _socket.write((char*) &_range_delim, _delim_len);
    _socket.write(num_of_rows_value, strlen(num_of_rows_value));
    _socket.write((char*) _delimeter, _delim_len);
    _socket.write((char*) READY_FOR_QUERY_KEY, strlen(READY_FOR_QUERY_KEY));
     writeSizeOfBlock(sizeof(uint32_t) + strlen(TRANZACTION_BLOCK_KEY));
    _socket.write((char*) TRANZACTION_BLOCK_KEY, strlen(TRANZACTION_BLOCK_KEY));   
}


void PgsqlWorker::writeSizeOfBlock(size_t size){
    uint32_t message_size = htonl(size);
    _socket.write((char*) &message_size, sizeof(message_size));
}

void PgsqlWorker::run() {
    const regex select_query_pattern("^SELECT.*");
    const regex insert_query_pattern("^INSERT.*");
    const regex update_query_pattern("^UPDATE.*");
    const regex delete_query_pattern("^DELETE.*");
    cmatch cm;
    char key;
    char* received_message;

    received_message = readMessage();
            
    _socket.write("N");

    received_message = readMessage();

    _socket.write(RESPONSE_MESSAGE_KEY,sizeof(RESPONSE_MESSAGE_KEY));

    //Not meaning end 
    unsigned char answer[1]={0xDE};



    unsigned char answer2[3]={_AUTH_REQ(AUTH_REQ_MD5), 0xF9, 0xF9};

    writeSizeOfBlock(sizeof(uint32_t) + sizeof(answer2));

    writeSizeOfBlock(sizeof(uint32_t) + sizeof(answer));

    _socket.read((char*) answer, sizeof(answer));

    _socket.write((char*) answer2, sizeof(answer2));

    _socket.read(&key, sizeof(key));

    getMessageSize();
    
    sendServerConfiguration();


    while(true){
        _socket.read(&key, sizeof(key));

        //The key of query
        if(key=='Q'){
            received_message = readMessage();

            if(0==strcmp(received_message, "BEGIN"))
                writeCodeAnswer("BEGIN",READY_FOR_QUERY_KEY, EMPTY_QUERY_RESPONSE_KEY);

            else if(0==strcmp(received_message, "COMMIT"))
                writeCodeAnswer("COMMIT",READY_FOR_QUERY_KEY, EMPTY_QUERY_RESPONSE_KEY);
       
            else if(regex_match(received_message,cm,select_query_pattern))
                sendSelectQueryResult(1, 1);

            else if(regex_match(received_message,cm,insert_query_pattern))
                sendInsertQueryResult(0, 1);

            else if(regex_match(received_message,cm,delete_query_pattern))
                sendDeleteQueryResult(1);

            else if(regex_match(received_message,cm,update_query_pattern))
                sendUpdateQueryResult(1);

            printf("%s\n", received_message);
        }

        //Close server message
        if(key=='X'){
            terminate_flag_local->store(true);
            getMessageSize();
            break;
        }
    }

    delete received_message;
}

/*
 **********************************************************************
 * PgsqlServer
 **********************************************************************
 */

PgsqlServer::PgsqlServer(std::atomic_bool* terminate_flag): TcpServer(&PgsqlWorkerThreadProc) {
    terminate_flag_local = terminate_flag;
}

PgsqlServer::~PgsqlServer(){
}

void* PgsqlServer::createWorkerThreadProcArg(int accepted_socket, std::atomic_int* workersCounter){
    sock = accepted_socket;

    auto arg = new(std::nothrow) PgsqlWorkerThreadProcArg();

    if(arg == nullptr) {
        close(accepted_socket);
        throw std::runtime_error("PgsqlServer::createWorkerThreadProcArg() - malloc() call failed");
    }
    arg->socket = accepted_socket;
    arg->workersCounter = workersCounter;
    
    return arg;
}
