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

static int __json_parse_string_length(const char* str, size_t *len) {
    char p[4];
    size_t ret, utf8_len;
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
                if ((ret = __json_parse_unicode(str, &str, p, &utf8_len)) != JSON_PARSE_OK) {
                    return ret;
                }
                *len += utf8_len;
                continue;
            }
        }
        str++;
        ++*len;
    }
    return JSON_PARSE_OK;
}

static int __json_parse_string_raw(const char* src, const char** end, char* dst) {
    size_t ret, utf8_len;
    char* p = dst;
    while (*src != '\"') {
        if (*src == '\\') {
            src++;
            switch (*src) {
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
                    src++;
                    if ((ret = __json_parse_unicode(src, &src, p, &utf8_len)) != JSON_PARSE_OK) {
                        return ret;
                    }
                    p += utf8_len;
                    continue;
                default:
                    return JSON_PARSE_INVALID_STRING_ESCAPE;
            }
        } else
            *p = *src;
        src++;
        p++;
    }
    *p = '\0';
    *end = src + 1;
    return JSON_PARSE_OK;
}

static int __json_parse_string_common(const char* src, const char** end, size_t* len, char** dst) {
    int ret;
    src++;
    if ((ret = __json_parse_string_length(src, len)) != JSON_PARSE_OK)
        return ret;
    *dst = (char*)malloc((*len + 1));
    assert(dst);
	if ((ret = __json_parse_string_raw(src, end, *dst)) != JSON_PARSE_OK) {
        free(*dst);
	}
    return ret;
}

static int __json_parse_string(const char* str, const char** end, json_value_t* v) {
    size_t len;
    int ret;
    len = 0;
    if ((ret = __json_parse_string_common(str, end, &len, &v->u.s.s)) != JSON_PARSE_OK)
        return ret;
    v->u.s.len = len;
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

static int __json_parse_value(const char* str, const char** end, json_value_t* v);

static int __json_parse_array(const char* str, const char** end, json_value_t* v) {
    json_value_t e, *curr;
    size_t i, size = 0;
    int ret = JSON_PARSE_OK;
    str++;
    while (is_whitespace(*str))
        str++;
    if (*str == ']') {
        *end = str + 1;
        v->type = JSON_ARRAY;
        v->u.a.e = NULL;
        v->u.a.size = 0;
        return ret;
    }
    for (;;) {
        json_init(&e);
        if ((ret = __json_parse_value(str, &str, &e)) != JSON_PARSE_OK) {
            break;
        }
        size++;
        if (size == 1) {
            v->u.a.e = (json_value_t*)malloc(sizeof(json_value_t));
        } else {
            v->u.a.e = (json_value_t*)realloc(v->u.a.e, size * sizeof(json_value_t));
        }
        curr = v->u.a.e + size - 1;
        memcpy(curr, &e, sizeof(json_value_t));
        while (is_whitespace(*str))
            str++;
        if (*str == ',') {
            str++;
            while (is_whitespace(*str))
                str++;
            if (*str == '\0') {
                ret = JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
                break;
            }
        } else if (*str == ']') {
            *end = str + 1;
            v->type = JSON_ARRAY;
            v->u.a.size = size;
            break;
        } else {
            ret =  JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }
    if (ret != JSON_PARSE_OK) {
        for (i = 0; i < size; i++)
            json_free(&v->u.a.e[i]);
        free(v->u.a.e);
        v->type = JSON_NULL;
    }
    return ret;
}

static void __json_init_member(json_member_t* m) {
    m->k = NULL;
    m->klen = 0;
    json_init(&m->v);
}

static int __json_parse_object(const char* str, const char** end, json_value_t* v) {
    json_member_t m, *curr;
    size_t i, size = 0;
    int ret = JSON_PARSE_OK;
    str++;
    while(is_whitespace(*str))
        str++;
    if (*str == '}') {
        *end = str + 1;
        v->type = JSON_OBJECT;
        v->u.o.size = 0;
        v->u.o.m = NULL;
        return ret;
    }
    for (;;) {
        __json_init_member(&m);
        if (*str != '\"') {
            ret = JSON_PARSE_MISS_KEY;
            break;
        }
        if ((ret = __json_parse_string_common(str, &str, &m.klen, &m.k)) != JSON_PARSE_OK)
            break;
        while (is_whitespace(*str))
            str++;
        if (*str != ':') {
            ret = JSON_PARSE_MISS_COLON;
            break;
        }
        str++;
        while (is_whitespace(*str))
            str++;
        if ((ret = __json_parse_value(str, &str, &m.v)) != JSON_PARSE_OK)
            break;
        size++;
        if (size == 1) {
            v->u.o.m = (json_member_t*)malloc(sizeof(json_member_t));
        } else {
            v->u.o.m = (json_member_t*)realloc(v->u.o.m, size * sizeof(json_member_t));
        }
        curr = v->u.o.m + size - 1;
        memcpy(curr, &m, sizeof(json_member_t));
        while(is_whitespace(*str))
            str++;
        if (*str == ',') {
            str++;
            while(is_whitespace(*str))
                str++;
        } else if (*str == '}') {
            *end = str + 1;
            v->type = JSON_OBJECT;
            v->u.o.size = size;
            break;
        } else {
            ret = JSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }
    if (ret != JSON_PARSE_OK) {
        for (i = 0; i < size; i++) {
            free(v->u.o.m->k);
            json_free(&v->u.o.m[i].v);
        }
        v->type = JSON_NULL;
    }
    return ret;
}

static int __json_parse_value(const char* str, const char** end, json_value_t* v) {
    switch (*str) {
        case 'n':  return __json_parse_literal(str, end, "null", JSON_NULL, v);
        case 't':  return __json_parse_literal(str, end, "true", JSON_TRUE, v);
        case 'f':  return __json_parse_literal(str, end, "false", JSON_FALSE, v);
        case '"':  return __json_parse_string(str, end, v);
        case '[':  return __json_parse_array(str, end, v);
        case '{':  return __json_parse_object(str, end, v);
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
    size_t i;
    assert(v != NULL);
    switch (v->type) {
        case JSON_STRING:
            free(v->u.s.s);
            break;
        case JSON_ARRAY:
            for (i = 0; i < v->u.a.size; i++)
                json_free(&v->u.a.e[i]);
            free(v->u.a.e);
        default: break;
    }
    v->type = JSON_NULL;
}

JSON_TYPE json_type(json_value_t* v) {
    assert(v != NULL);
    return v->type;
}

char* json_get_string(const json_value_t* v) {
    assert(v != NULL && v->type == JSON_STRING);
    return v->u.s.s;
}
size_t json_get_string_length(const json_value_t* v) {
    assert(v != NULL && v->type == JSON_STRING);
    return v->u.s.len;
}

double json_get_number(const json_value_t* v) {
    assert(v != NULL && v->type == JSON_NUMBER);
    return v->u.n;
}

size_t json_get_array_size(const json_value_t* v) {
    assert(v != NULL && v->type == JSON_ARRAY);
    return v->u.a.size;
}

json_value_t* json_get_array_element(const json_value_t* v, size_t index) {
    assert(v != NULL && v->type == JSON_ARRAY);
    assert(index < v->u.a.size);
    return &v->u.a.e[index];
}

size_t json_get_object_size(const json_value_t* v) {
    assert(v != NULL && v->type == JSON_OBJECT);
    return v->u.o.size;
}

const char* json_get_object_key(const json_value_t* v, size_t index) {
    assert(v != NULL && v->type == JSON_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].k;
}

size_t json_get_object_key_length(const json_value_t* v, size_t index) {
    assert(v != NULL && v->type == JSON_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].klen;
}

json_value_t* json_get_object_value(const json_value_t* v, size_t index) {
    assert(v != NULL && v->type == JSON_OBJECT);
    assert(index < v->u.o.size);
    return &v->u.o.m[index].v;
}