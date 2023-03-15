#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "hadrjson.h"

static bool is_whitespace(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static int __json_parse_literal(const char* str, const char** end, const char *literal, JSON_TYPE type, json_value_t* v) {
    size_t len;
    len = strlen(literal);
    if (strncmp(str, literal, len))
        return JSON_PARSE_INVALID_VALUE;
    *end = str + len;
    v->type = type;
    return JSON_PARSE_OK;
}

static int __json_parse_string_length(const char* str, size_t *len) {
    for (;;) {
        if (*str == '\0')
            return JSON_PARSE_MISS_QUOTATION_MARK;
        if (*str == '\"')
            break;
        if (*str == '\\') {
            str++;
            if (!*str)
                return JSON_PARSE_INVALID_STRING_ESCAPE;
        }
        str++;
        (*len)++;
    }
    return JSON_PARSE_OK;
}

static int __json_parse_string_raw(const char* str, const char** end, json_value_t* v) {
    char* p = v->u.s.s;
    while (*str != '\"') {
        if (*str == '\\') {
            str++;
            switch (*str) {
                case '\"':
                    *p = '\"';
                    break;
                case '\\':
                    *p = '\\';
                    break;
                case '/':
                    *p = '/';
                    break;
                case 'b':
                    *p = '\b';
                    break;
                case 'f':
                    *p = '\f';
                    break;
                case 'n':
                    *p = '\n';
                    break;
                case 'r':
                    *p = '\r';
                    break;
                case 't':
                    *p = '\t';
                    break;
                case 'u':
                default:
                    return JSON_PARSE_INVALID_STRING_ESCAPE;
            }
        } else if ((unsigned char)*str < 0x20) {
            return JSON_PARSE_INVALID_STRING_CHAR;
        } else
            *p = *str;
        str++;
        p++;
    }
    *p = '\0';
    *end = str + 1;
    return JSON_PARSE_OK;
}

static int __json_parse_string(const char* str, const char** end, json_value_t* v) {
    size_t len;
    int ret;
    str++;
    len = 0;
    if ((ret = __json_parse_string_length(str, &len)) != JSON_PARSE_OK)
        return ret;
    v->u.s.s = (char*)malloc((len + 1));
    assert(v->u.s.s);
    v->u.s.len = len;
	if ((ret = __json_parse_string_raw(str, end, v)) != JSON_PARSE_OK) {
		free(v->u.s.s);
		return ret;
	}
    v->type = JSON_STRING;
    return ret;
}

static int __json_parse_value(const char* str, const char** end, json_value_t* v) {
    switch (*str) {
        case 'n':  return __json_parse_literal(str, end, "null", JSON_NULL, v);
        case 't':  return __json_parse_literal(str, end, "true", JSON_TRUE, v);
        case 'f':  return __json_parse_literal(str, end, "false", JSON_FALSE, v);
        case '"':  return __json_parse_string(str, end, v);
        case '\0': return JSON_PARSE_EXPECT_VALUE;
        default:   return JSON_PARSE_INVALID_VALUE;
    }
}

int json_parse(json_value_t* v, const char* str) {
    int ret;
    assert(v != NULL);
    json_init(v);
    while (is_whitespace(*str))
        str++;
    if ((ret = __json_parse_value(str, &str, v)) == JSON_PARSE_OK) {
        while (is_whitespace(*str))
            str++;
        if (*str != '\0') {
            json_free(v);
            ret = JSON_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

void json_free(json_value_t* v) {
    assert(v != NULL);
    if (v->type == JSON_STRING)
        free(v->u.s.s);
    v->type = JSON_NULL;
}

char* json_get_string(json_value_t* v) {
    assert(v != NULL && v->type == JSON_STRING);
    return v->u.s.s;
}
size_t json_get_string_length(json_value_t* v) {
    assert(v != NULL && v->type == JSON_STRING);
    return v->u.s.len;
}

JSON_TYPE json_type(json_value_t* v) {
    assert(v != NULL);
    return v->type;
}