#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "hadrjson.h"

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

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

static int __json_parse_hex(const char* str, const char **end, unsigned int* u);

static int __json_parse_string_length(const char* str, size_t *len) {
    size_t ret;
    unsigned u;
    for (;;) {
        if (*str == '\0')
            return JSON_PARSE_MISS_QUOTATION_MARK;
        if (*str == '\"')
            break;
        if (*(unsigned char*)str < 0x20)
            return JSON_PARSE_INVALID_STRING_CHAR;
        if (*str == '\\') {
            str++;
            if (!*str)
                return JSON_PARSE_INVALID_STRING_ESCAPE;
            if (*str == 'u') {
                str++;
                if ((ret = __json_parse_hex(str, &str, &u)) != JSON_PARSE_OK) {
                    return ret;
                }
                ++*len;
                continue;
            }
        }
        str++;
        ++*len;
    }
    return JSON_PARSE_OK;
}

static int __json_parse_hex(const char* str, const char **end, unsigned int* u) {
    int i;
    *u = 0;
    for (i = 0; i < 4; i++) {
        char ch = *str++;
        *u <<= 4;
        if      (ch >= '0' && ch <= '9') *u |= ch - '0';
        else if (ch >= 'a' && ch <= 'f') *u |= ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F') *u |= ch - 'A' + 10;
        else 
            return JSON_PARSE_INVALID_UNICODE_HEX;
    }
    *end = str;
    return JSON_PARSE_OK;
}

static size_t __json_encode_utf8(unsigned int u, char* p) {
    size_t utf8_len = 0;
    if (u <= 0x007F) {
        p[0] = u;
        utf8_len = 1;
    } else if (u <= 0x07FF) {
        p[0] = 0xc0 | ((u >> 6)  & 0xff);
        p[1] = 0x80 | ( u        & 0x3f);
        utf8_len = 2;
    } else if (u <= 0xFFFF) {
        p[0] = 0xe0 | ((u >> 12) & 0xff);
        p[1] = 0x80 | ((u >> 6)  & 0x3f);
        p[2] = 0x80 | ( u        & 0x3f);
        utf8_len = 3;
    } else {
        assert(u <= 0x10FFFF);
        p[0] = 0xf0 | ((u >> 18) & 0xff);
        p[1] = 0x80 | ((u >> 12) & 0x3f);
        p[2] = 0x80 | ((u >> 6 ) & 0x3f);
        p[3] = 0x80 | ( u        & 0x3f);
        utf8_len = 4;
    }
    return utf8_len;
}

static int __json_parse_unicode(const char* str, const char** end, char* p, size_t* utf8_len) {
    size_t ret;
    unsigned int u, u2;
    if ((ret = __json_parse_hex(str, end, &u)) != JSON_PARSE_OK)
        return ret;
    if (u >= 0xD800 && u <= 0xDBFF) {
        str = *end;
        if (*str++ != '\\') return JSON_PARSE_INVALID_UNICODE_SURROGATE;
        if (*str++ != 'u') return JSON_PARSE_INVALID_UNICODE_SURROGATE;
        if ((ret = __json_parse_hex(str, end, &u2)) != JSON_PARSE_OK)
            return ret;
        if (u2 <= 0xDC00 || u2 > 0xDFFF) return JSON_PARSE_INVALID_UNICODE_SURROGATE;
        u = 0x10000 + (((u - 0xD800) << 10) | (u2 - 0xDC00));
    }
    *utf8_len = __json_encode_utf8(u, p);
    return JSON_PARSE_OK;
}

static int __json_parse_string_raw(const char* str, const char** end, json_value_t* v) {
    size_t ret, utf8_len;
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
                    str++;
                    if ((ret = __json_parse_unicode(str, &str, p, &utf8_len)) != JSON_PARSE_OK) {
                        return ret;
                    }
                    p += utf8_len;
                    v->u.s.len += utf8_len > 2 ? 2 : (utf8_len == 2 ? 1 : 0);
                    continue;
                default:
                    return JSON_PARSE_INVALID_STRING_ESCAPE;
            }
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

static int __json_parse_number(const char* str, const char** end, json_value_t* v) {
    const char* p = str;
    if (*p == '-')
        p++;
    if (*p == '0') {
        if (ISDIGIT(p[1]) || p[1] == 'x' || p[1] == 'X')
            return JSON_PARSE_ROOT_NOT_SINGULAR;
        p++;
    } else if (ISDIGIT1TO9(*p)) {
        for (p++; ISDIGIT(*p); p++);
    } else {
        return JSON_PARSE_INVALID_VALUE;
    }
    if (*p == '.') {
        if (!ISDIGIT(*++p))
            return JSON_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        for (; ISDIGIT(*p); p++);
    }
    errno = 0;
    v->u.n = strtod(str, (char**)end);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return JSON_PARSE_NUMBER_TOO_BIG;
    v->type = JSON_NUMBER;
    return JSON_PARSE_OK;
}

static int __json_parse_value(const char* str, const char** end, json_value_t* v) {
    switch (*str) {
        case 'n':  return __json_parse_literal(str, end, "null", JSON_NULL, v);
        case 't':  return __json_parse_literal(str, end, "true", JSON_TRUE, v);
        case 'f':  return __json_parse_literal(str, end, "false", JSON_FALSE, v);
        case '"':  return __json_parse_string(str, end, v);
        default:   return __json_parse_number(str, end, v);
        case '\0': return JSON_PARSE_EXPECT_VALUE;
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

double json_get_number(json_value_t* v) {
    assert(v != NULL && v->type == JSON_NUMBER);
    return v->u.n;
}

JSON_TYPE json_type(json_value_t* v) {
    assert(v != NULL);
    return v->type;
}