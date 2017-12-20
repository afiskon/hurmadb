/* vim: set ai et ts=4 sw=4: */

#include <TcpServer.h>
#include <PgsqlServer.h>
#include <RegexCache.h>
#include <Socket.h>
#include <arpa/inet.h>
#include <atomic>
#include <defer.h>
#include <deque>
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
        const unsigned char successful_auth[] = {0x00, 0x00, 0x00, 0x00};

        _socket.write((char*) RESPONSE_MESSAGE_KEY, sizeof(RESPONSE_MESSAGE_KEY));

        writeSizeOfBlock(sizeof(uint32_t) + _delim_len * 4);

        _socket.write((char*) successful_auth, sizeof(successful_auth));

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

void PgsqlWorker::sendSelectQueryResult(int16_t num_of_columns, int16_t num_of_rows, vector<DataColumn> columns, deque<vector<string>> rows){
    const char* select_command = "SELECT";
    const string num_of_rows_buf = to_string(num_of_rows);
    const char* num_of_rows_string = num_of_rows_buf.c_str();
    const int16_t l_num_of_columns = htons(num_of_columns);
    int columns_len = 0;

    _socket.write((char*) &ROW_DESCRIPTION_KEY, sizeof(ROW_DESCRIPTION_KEY));

    for(int i = 0; i<num_of_columns; i++){
        columns_len += strlen(columns[i].column_name);
        columns_len += sizeof(columns[i].spec_col_id);
        columns_len += sizeof(columns[i].num_of_column);
        columns_len += sizeof(columns[i].data_type_id);
        columns_len += sizeof(columns[i].data_type_size);
        columns_len += sizeof(columns[i].type_modifier);
        columns_len += sizeof(columns[i].code_format);
    }

    writeSizeOfBlock(sizeof(uint32_t) + sizeof(l_num_of_columns) + columns_len + _delim_len);
    _socket.write((char*) &l_num_of_columns, sizeof(l_num_of_columns));

    // In cycle, depends of number of columns(fields)
    for(int i = 0; i<num_of_columns; i++){
        DataColumn dc = columns[i];
        _socket.write((char*) dc.column_name, strlen(dc.column_name));
        _socket.write((char*) &dc.spec_col_id, sizeof(dc.spec_col_id));
        _socket.write((char*) &dc.num_of_column, sizeof(dc.num_of_column));
        _socket.write((char*) &dc.data_type_id, sizeof(dc.data_type_id));
        _socket.write((char*) &dc.data_type_size, sizeof(dc.data_type_size));
        _socket.write((char*) &dc.type_modifier, sizeof(dc.type_modifier));
        _socket.write((char*) &dc.code_format, sizeof(dc.code_format));
        _socket.write((char*) _delimeter, _delim_len);
    }
    //End of cycle
    //Write data rows
    while(rows.size()>0){
        vector<string> cur_row = rows.front();
        rows.pop_front();
        
        int size_of_values = 0;
        for(int i=0; i<num_of_columns; i++){
            size_of_values += cur_row[i].length() + sizeof(uint32_t);
        }
        _socket.write((char*) &RESULTED_ROWS_DELIMITER, sizeof(RESULTED_ROWS_DELIMITER));
        writeSizeOfBlock(sizeof(uint32_t) + size_of_values + sizeof(l_num_of_columns));
        _socket.write((char*) &l_num_of_columns, sizeof(l_num_of_columns));

        for(int i=0; i<num_of_columns; i++){
            writeSizeOfBlock(cur_row[i].length());
            _socket.write((char*) cur_row[i].c_str(), cur_row[i].length());
        }
        
    }
    
    /* Code message part */
    _socket.write((char*)COMMAND_KEY, sizeof(COMMAND_KEY));
    writeSizeOfBlock(sizeof(uint32_t) +
            strlen(select_command) + 
            _delim_len + 
            strlen(num_of_rows_string) + 
            strlen(READY_FOR_QUERY_KEY)
        );
    _socket.write(select_command, strlen(select_command));
    _socket.write((char*) &_range_delim, _delim_len);
    _socket.write(num_of_rows_string, strlen(num_of_rows_string));
    _socket.write((char*) _delimeter, _delim_len);
    writeReadyForQueryMessage(TRANZACTION_BLOCK_KEY, sizeof(TRANZACTION_BLOCK_KEY));   
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
    writeReadyForQueryMessage(TRANZACTION_BLOCK_KEY, sizeof(TRANZACTION_BLOCK_KEY));   
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
    writeReadyForQueryMessage(TRANZACTION_BLOCK_KEY, sizeof(TRANZACTION_BLOCK_KEY));  
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
    writeReadyForQueryMessage(TRANZACTION_BLOCK_KEY, sizeof(TRANZACTION_BLOCK_KEY)); 
}

/*
Send to client next message: 
    C...UPDATE.num_of_rows.Z...T
where num_of_rows is the number of updated rows
*/
void PgsqlWorker::sendErrorMessage(){
    _socket.write((char*)&ERROR_RESPONSE_KEY, sizeof(ERROR_RESPONSE_KEY));
}


void PgsqlWorker::writeReadyForQueryMessage(const char key, int key_size){
    _socket.write((char*) READY_FOR_QUERY_KEY, strlen(READY_FOR_QUERY_KEY));
    writeSizeOfBlock(sizeof(uint32_t) + key_size);
    _socket.write(&key, key_size);  
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
    const unsigned char pass_delimiter[1]={0xDE};
    const unsigned char pass_type[1]={_AUTH_REQ(AUTH_REQ_MD5)};
    const unsigned char noise[2]={0xF9, 0xF9};
    cmatch cm;
    char key;
    char* received_message;

    received_message = readMessage();
            
    _socket.write((char*) &NOTICE_RESPONSE_KEY, sizeof(NOTICE_RESPONSE_KEY));

    received_message = readMessage();

    _socket.write(RESPONSE_MESSAGE_KEY,sizeof(RESPONSE_MESSAGE_KEY));

    writeSizeOfBlock(
        sizeof(uint32_t) + 
        sizeof(uint32_t) + 
        sizeof(pass_delimiter) + 
        sizeof(pass_type) + 
        sizeof(noise)
        );

    writeSizeOfBlock(sizeof(uint32_t) + sizeof(pass_delimiter));

    _socket.write((char*) pass_delimiter, sizeof(pass_delimiter));
    _socket.write((char*) pass_type, sizeof(pass_type));
    _socket.write((char*) noise, sizeof(noise));
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
       
            else if(regex_match(received_message,cm,select_query_pattern)){
                vector<DataColumn> columns;
                const char* c_name = "col1";
                DataColumn dc(c_name);
                columns.push_back(dc);
                
                deque<vector<string>> rows;
                vector<string> row;
                row.push_back("151");
                row.push_back("value");
                rows.push_front(row);
                sendSelectQueryResult(1, 1, columns, rows);
            }

            else if(regex_match(received_message,cm,insert_query_pattern))
                sendInsertQueryResult(1, 1);

            else if(regex_match(received_message,cm,delete_query_pattern))
                sendDeleteQueryResult(1);

            else if(regex_match(received_message,cm,update_query_pattern))
                sendUpdateQueryResult(1);

            printf("%s\n", received_message);
            defer(delete received_message);
        }

        //Close server message
        if(key=='X'){
            terminate_flag_local->store(true);
            getMessageSize();
            break;
        }
    }

   
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