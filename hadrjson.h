#ifndef _HADRJSON_H_
#define _HADRJSON_H_

#include <stddef.h>

typedef enum {
    JSON_NULL = 1,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JSON_TYPE;

enum {
    JSON_PARSE_OK = 0,
    JSON_PARSE_EXPECT_VALUE,
    JSON_PARSE_INVALID_VALUE,
    JSON_PARSE_NUMBER_TOO_BIG,
    JSON_PARSE_ROOT_NOT_SINGULAR,
    JSON_PARSE_MISS_QUOTATION_MARK,
    JSON_PARSE_INVALID_STRING_ESCAPE,
    JSON_PARSE_INVALID_STRING_CHAR,
    JSON_PARSE_INVALID_UNICODE_HEX,
    JSON_PARSE_INVALID_UNICODE_SURROGATE,
    JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET
};

typedef struct json_member_t json_member_t;
typedef struct json_value_t json_value_t;

struct json_value_t {
    union {
        double n;
        struct { char* s; size_t len; } s;
        struct { json_value_t*  e; size_t size, capacity; } a;
        struct { json_member_t* m; size_t size, capacity; } o;
    } u;
    JSON_TYPE type;
};

struct json_member_t {
    char* k;
    size_t klen;
    json_value_t* v;
};

#define json_init(v) do { (v)->type = JSON_NULL; } while(0)
int json_parse(json_value_t* v, const char* str);
void json_free(json_value_t* v);

JSON_TYPE json_type(json_value_t* v);

double json_get_number(const json_value_t* v);

char* json_get_string(const json_value_t* v);
size_t json_get_string_length(const json_value_t* v);

size_t json_get_array_size(const json_value_t* v);
json_value_t* json_get_array_element(const json_value_t* v, size_t index);
#endif