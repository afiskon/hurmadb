/* vim: set ai et ts=4 sw=4: */
#include <DataColumn.h>
#include <cstdint>

DataColumn::DataColumn(const char* column_name,
                       uint32_t spec_col_id,
                       int16_t num_of_column,
                       int32_t data_type_id,
                       int16_t data_type_size,
                       int32_t type_modifier,
                       int16_t code_format) {
    this->column_name = column_name;
    this->spec_col_id = spec_col_id;
    this->num_of_column = num_of_column;
    this->data_type_id = data_type_id;
    this->data_type_size = data_type_size;
    this->type_modifier = type_modifier;
    this->code_format = code_format;
}

DataColumn::DataColumn(const char* column_name) {
    this->column_name = column_name;
    this->spec_col_id = 0;
    this->num_of_column = 0;
    this->data_type_id = 0;
    this->data_type_size = 0;
    this->type_modifier = 0;
    this->code_format = 0;
}

DataColumn::DataColumn() {
}