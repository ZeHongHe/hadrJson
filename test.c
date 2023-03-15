#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hadrjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect == actual), expect, actual, "%d")
#define EXPECT_EQ_STRING(expect, actual, length) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == (length) && !memcmp(expect, actual, length), expect, actual, "%s")

#define TEST_LITERAL(expect, json)\
    do {\
        json_value_t v;\
        json_init(&v);\
        EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&v, json));\
        EXPECT_EQ_INT(expect, json_type(&v));\
        json_free(&v);\
    } while(0)

static void test_parse_literal() {
    TEST_LITERAL(JSON_NULL, "null");
    TEST_LITERAL(JSON_TRUE, "true");
    TEST_LITERAL(JSON_FALSE, "false");
}

#define TEST_STRING(expect, json)\
    do {\
        json_value_t v;\
        json_init(&v);\
        EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&v, json));\
        EXPECT_EQ_INT(JSON_STRING, json_type(&v));\
        EXPECT_EQ_STRING(expect, json_get_string(&v), json_get_string_length(&v));\
        json_free(&v);\
    } while(0)

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
}

#define TEST_ERROR(error, json)\
    do {\
        json_value_t v;\
        json_init(&v);\
        v.type = JSON_FALSE;\
        EXPECT_EQ_INT(error, json_parse(&v, json));\
        EXPECT_EQ_INT(JSON_NULL, json_type(&v));\
        json_free(&v);\
    } while(0)

static void test_parse_expect_value() {
    TEST_ERROR(JSON_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(JSON_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "?");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "tru");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "fals");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(JSON_PARSE_ROOT_NOT_SINGULAR, "null n");
}

static void test_parse_missing_quotation_mark() {
    TEST_ERROR(JSON_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(JSON_PARSE_MISS_QUOTATION_MARK, "\"Hello");
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(JSON_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(JSON_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

void test_parse() {
    test_parse_literal();
    test_parse_string();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}