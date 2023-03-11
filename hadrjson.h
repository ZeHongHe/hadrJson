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

typedef struct json_member_t json_member_t;
typedef struct json_value_t json_value_t;

struct json_value_t {
    union {
        double n;
        struct { char* s; size_t len; } s;
        struct { json_value_t* v; size_t size; } a;
        struct { json_member_t* m; size_t size; } o;
    } u;
    JSON_TYPE type;
};

struct json_member_t {
    char* k;
    size_t klen;
    json_value_t* v;
};

json_value_t* json_parse(const char* s);
void json_destory(json_value_t* v);

char* json_get_string(json_value_t* v);
size_t json_get_string_length(json_value_t* v);
JSON_TYPE json_type(json_value_t* v);

#endif