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

#define TEST_STRING(expect, actual)\
    do {\
        json_value_t *v;\
        v = json_parse(actual);\
        EXPECT_EQ_INT(JSON_STRING, json_type(v));\
        EXPECT_EQ_STRING(expect, json_get_string(v), json_get_string_length(v));\
    } while (0)


void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\" \\\\ \\/ \\b \\f \\n \\r \\t \"");
}

void test_parse() {
    test_parse_string();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}