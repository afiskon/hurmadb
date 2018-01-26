/* vim: set ai et ts=4 sw=4: */

#pragma once
#include <cstdint>
#include <stdio.h>
#include <string>

using namespace std;

/*
    This class is the error message of pgsqlserver, emulated error message of postgresql.
    But here realized fields, which required only for curremt versin of pgsqlserver, to
    see all fields go to https://www.postgresql.org/docs/10/static/protocol-error-fields.html
*/
class ErrorMesage {

public:
    ErrorMesage(char* server_version);
    ErrorMesage(const char* server_version, char* s_severity, char* code, string message);
    ErrorMesage();
    const char* getSSeverity();
    const char* getVSeverity();
    int getSSeveritySize();
    int getVSeveritySize();
    bool isVSeverityAvailable();
    const char* getCode();
    int getCodeSize();
    const char* getMessage();
    int getMessageSize();
    int getSizeOfFullErrorMessage();

private:
    const char* v_version = "9.6";
    const char s_severity_token = 'S';
    const char v_severity_token = 'V';
    const char code_token = 'C';
    const char mesage_token = 'M';

    bool newest_version;
    string s_severity;
    string v_severity;
    string code;
    string message;
};