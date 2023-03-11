#include <stdlib.h>
#include <stddef.h>
#include "hadrjson.h"

json_value_t* json_parse(const char* s) {
    return NULL;
}

void json_destory(json_value_t* v) {

}

char* json_get_string(json_value_t* v) {
    return v->u.s.s;
}
size_t json_get_string_length(json_value_t* v) {
    return v->u.s.len;
}

JSON_TYPE json_type(json_value_t* v) {
    return v->type;
}