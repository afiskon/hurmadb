/* vim: set ai et ts=4 sw=4: */
#include <ErrorMesage.h>
#include <stdio.h>
#include <string.h>

ErrorMesage::ErrorMesage(char* server_version) {
    if(strcmp(server_version, v_version) == 0)
        newest_version = true;
    else
        newest_version = false;
}

ErrorMesage::ErrorMesage(const char* server_version, char* s_severity, char* code, string message) {
    this->s_severity = s_severity;
    if(strcmp(server_version, v_version) >= 0) {
        this->v_severity = s_severity;
        newest_version = true;
    } else
        newest_version = false;

    this->code = code;
    this->message = message;
}

ErrorMesage::ErrorMesage() {
}

const char* ErrorMesage::getSSeverity() {
    s_severity = s_severity_token + s_severity;
    return s_severity.c_str();
}

int ErrorMesage::getSSeveritySize() {
    return sizeof(s_severity_token) + s_severity.length();
}

const char* ErrorMesage::getVSeverity() {
    v_severity = v_severity_token + v_severity;
    return v_severity.c_str();
}

int ErrorMesage::getVSeveritySize() {
    return sizeof(s_severity_token) + v_severity.length();
}

const char* ErrorMesage::getCode() {
    code = code_token + code;
    return code.c_str();
}

int ErrorMesage::getCodeSize() {
    return sizeof(code_token) + code.length();
}

const char* ErrorMesage::getMessage() {
    message = mesage_token + message;
    return message.c_str();
}

int ErrorMesage::getMessageSize() {
    return sizeof(mesage_token) + message.length();
}

bool ErrorMesage::isVSeverityAvailable() {
    return newest_version;
}

int ErrorMesage::getSizeOfFullErrorMessage() {
    int size = getSSeveritySize() + getCodeSize() + getMessageSize();
    if(newest_version) {
        size += getVSeveritySize();
    }
    return size;
}
