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

typedef __json_member json_member_t;
typedef __json_value json_value_t;

struct __json_value {
    union {
        double n;
        struct { char* s; size_t len; } string;
        struct { json_value_t* v; size_t size; } array;
        struct { json_member_t* m; size_t size; } object;
    } value;
    JSON_TYPE type;
};

struct __json_member {
    char* k;
    size_t klen;
    json_value_t* v;
};

json_value_t* json_parse(const char* s);
void json_destory(json_value_t* v);

int json_type(json_value_t* v);

#endif