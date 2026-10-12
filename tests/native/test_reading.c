#include "serialledger_c.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "Failed at line %d: %s\n", __LINE__, #condition); return 1; } } while (0)

typedef struct {
    char data[SL_MAX_LINE_BYTES + 2u];
    size_t used;
    size_t limit;
} sink;

static size_t capture(void *user, const unsigned char *data, size_t size)
{
    sink *output = (sink *)user;
    const size_t count = size < output->limit ? size : output->limit;
    if (count > sizeof(output->data) - 1u - output->used) {
        return 0u;
    }
    memcpy(output->data + output->used, data, count);
    output->used += count;
    output->data[output->used] = '\0';
    return count;
}

int main(void)
{
    char buffer[512];
    char tiny[8];
    char large_buffer[SL_MAX_LINE_BYTES + 1u];
    char large_value[SL_MAX_LINE_BYTES];
    const char *invalid_utf8[] = {
        "\xc0\xaf", "\xe2", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe0\x80\xaf"
    };
    sl_reading reading;
    sl_logger logger;
    sink output = {{0}, 0u, 3u};
    size_t index;
    CHECK(sl_init(&logger, capture, &output) == SL_OK);
    CHECK(sl_reading_init(NULL, buffer, sizeof(buffer)) == SL_INVALID_ARGUMENT);
    CHECK(sl_reading_add_int(NULL, "a", 1) == SL_INVALID_ARGUMENT);
    CHECK(sl_reading_send(&logger, NULL) == SL_INVALID_ARGUMENT);
    CHECK(sl_reading_init(&reading, NULL, 10u) == SL_INVALID_ARGUMENT);
    CHECK(sl_reading_send(&logger, &reading) == SL_INVALID_ARGUMENT);
    CHECK(sl_reading_init(&reading, tiny, 2u) == SL_BUFFER_TOO_SMALL);

    CHECK(sl_reading_init(&reading, buffer, sizeof(buffer)) == SL_OK);
    CHECK(sl_reading_send(&logger, &reading) == SL_INVALID_ARGUMENT);
    CHECK(sl_reading_add_int(&reading, "signed", INT64_MIN) == SL_OK);
    CHECK(sl_reading_add_uint(&reading, "unsigned", UINT64_MAX) == SL_OK);
    CHECK(sl_reading_add_float(&reading, "temperature_C", -12.375, 2u) == SL_OK);
    CHECK(sl_reading_add_bool(&reading, "enabled", 1) == SL_OK);
    CHECK(sl_reading_add_bool(&reading, "disabled", 0) == SL_OK);
    CHECK(sl_reading_add_null(&reading, "missing") == SL_OK);
    CHECK(sl_reading_send(&logger, &reading) == SL_OK);
    CHECK(strcmp(output.data,
        "{\"signed\":-9223372036854775808,\"unsigned\":18446744073709551615,"
        "\"temperature_C\":-12.38,\"enabled\":true,\"disabled\":false,\"missing\":null}\n") == 0);

    CHECK(sl_reading_init(&reading, buffer, sizeof(buffer)) == SL_OK);
    CHECK(sl_reading_add_string(&reading, "lab\"el\n", "a\"b\\c\t\r\n\x01\xe2\x98\x83") == SL_OK);
    CHECK(strcmp(buffer,
        "{\"lab\\\"el\\u000a\":\"a\\\"b\\\\c\\u0009\\u000d\\u000a\\u0001\xe2\x98\x83\"}") == 0);
    CHECK(sl_reading_add_string(&reading, "empty", "") == SL_OK);
    CHECK(sl_reading_add_string(&reading, "emoji", "\xf0\x9f\x98\x80") == SL_OK);
    output.used = 0u;
    CHECK(sl_reading_send(&logger, &reading) == SL_OK);
    CHECK(output.used == strlen(buffer) + 1u);

    CHECK(sl_reading_init(&reading, buffer, sizeof(buffer)) == SL_OK);
    CHECK(sl_reading_add_float(&reading, "carry", 9.999, 2u) == SL_OK);
    CHECK(sl_reading_add_float(&reading, "zero", -0.004, 2u) == SL_OK);
    CHECK(sl_reading_add_float(&reading, "integer", -0.5, 0u) == SL_OK);
    CHECK(sl_reading_add_float(&reading, "max", SL_MAX_FLOAT_MAGNITUDE, 6u) == SL_OK);
    CHECK(strcmp(buffer,
        "{\"carry\":10.00,\"zero\":0.00,\"integer\":-1,\"max\":1000000000.000000}") == 0);

    CHECK(sl_reading_init(&reading, tiny, sizeof(tiny)) == SL_OK);
    CHECK(sl_reading_add_int(&reading, "a", 1) == SL_OK);
    CHECK(strcmp(tiny, "{\"a\":1}") == 0);
    CHECK(sl_reading_add_int(&reading, "b", 2) == SL_BUFFER_TOO_SMALL);
    CHECK(strcmp(tiny, "{\"a\":1}") == 0);
    CHECK(sl_reading_add_null(&reading, "c") == SL_BUFFER_TOO_SMALL);
    output.used = 0u;
    CHECK(sl_reading_send(&logger, &reading) == SL_BUFFER_TOO_SMALL);
    CHECK(output.used == 0u);
    CHECK(sl_reading_init(&reading, tiny, sizeof(tiny) - 1u) == SL_OK);
    CHECK(sl_reading_add_int(&reading, "a", 1) == SL_BUFFER_TOO_SMALL);
    CHECK(strcmp(tiny, "{}") == 0);

    for (index = 0u; index < sizeof(invalid_utf8) / sizeof(invalid_utf8[0]); ++index) {
        CHECK(sl_reading_init(&reading, buffer, sizeof(buffer)) == SL_OK);
        CHECK(sl_reading_add_string(&reading, "text", invalid_utf8[index]) == SL_INVALID_ARGUMENT);
        CHECK(strcmp(buffer, "{}") == 0);
        CHECK(sl_reading_send(&logger, &reading) == SL_INVALID_ARGUMENT);
    }
    CHECK(sl_reading_init(&reading, buffer, sizeof(buffer)) == SL_OK);
    CHECK(sl_reading_add_string(&reading, "text", buffer) == SL_INVALID_ARGUMENT);
    CHECK(sl_reading_init(&reading, buffer, sizeof(buffer)) == SL_OK);
    CHECK(sl_reading_add_int(&reading, "", 1) == SL_INVALID_ARGUMENT);
    CHECK(sl_reading_init(&reading, buffer, sizeof(buffer)) == SL_OK);
    CHECK(sl_reading_add_string(&reading, "a", NULL) == SL_INVALID_ARGUMENT);
    CHECK(sl_reading_init(&reading, buffer, sizeof(buffer)) == SL_OK);
    CHECK(sl_reading_add_float(&reading, "a", NAN, 2u) == SL_INVALID_ARGUMENT);
    CHECK(sl_reading_send(&logger, &reading) == SL_INVALID_ARGUMENT);
    CHECK(sl_reading_init(&reading, buffer, sizeof(buffer)) == SL_OK);
    CHECK(sl_reading_add_float(&reading, "a", INFINITY, 2u) == SL_VALUE_OUT_OF_RANGE);
    CHECK(sl_reading_init(&reading, buffer, sizeof(buffer)) == SL_OK);
    CHECK(sl_reading_add_float(&reading, "a", SL_MAX_FLOAT_MAGNITUDE + 1.0, 2u) == SL_VALUE_OUT_OF_RANGE);
    CHECK(sl_reading_init(&reading, buffer, sizeof(buffer)) == SL_OK);
    CHECK(sl_reading_add_float(&reading, "a", 1.0, 7u) == SL_INVALID_ARGUMENT);

    memset(large_value, 'x', sizeof(large_value));
    large_value[SL_MAX_LINE_BYTES - 8u] = '\0';
    CHECK(sl_reading_init(&reading, large_buffer, sizeof(large_buffer)) == SL_OK);
    CHECK(sl_reading_add_string(&reading, "s", large_value) == SL_OK);
    CHECK(reading.length == SL_MAX_LINE_BYTES);
    CHECK(sl_reading_send(&logger, &reading) == SL_OK);
    CHECK(output.used == SL_MAX_LINE_BYTES + 1u);
    large_value[SL_MAX_LINE_BYTES - 8u] = 'x';
    large_value[SL_MAX_LINE_BYTES - 7u] = '\0';
    CHECK(sl_reading_init(&reading, large_buffer, sizeof(large_buffer)) == SL_OK);
    CHECK(sl_reading_add_string(&reading, "s", large_value) == SL_LINE_TOO_LONG);
    CHECK(strcmp(large_buffer, "{}") == 0);

    CHECK(sl_reading_init(&reading, buffer, sizeof(buffer)) == SL_OK);
    CHECK(sl_reading_add_bool(&reading, "ready", 1) == SL_OK);
    output.limit = 0u;
    CHECK(sl_reading_send(&logger, &reading) == SL_WRITE_FAILED);
    return 0;
}
