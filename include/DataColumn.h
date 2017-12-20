/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <cstdint>

class DataColumn {
public:
    const char* column_name;
    uint32_t spec_col_id;
    int16_t num_of_column;
    int32_t data_type_id;
    int16_t data_type_size;
    int32_t type_modifier;
    int16_t code_format; 

    DataColumn(const char* column_name, uint32_t spec_col_id, int16_t num_of_column, int32_t data_type_id, int16_t data_type_size, int32_t type_modifier, int16_t code_format);
    DataColumn(const char* column_name);
    DataColumn();
};