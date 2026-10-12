#include "serialledger_c.h"
#include <stdio.h>
#include <string.h>
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "Failed at line %d: %s\n", __LINE__, #condition); return 1; } } while (0)

typedef struct {
    unsigned char data[SL_MAX_LINE_BYTES + 2u];
    size_t used;
    size_t limit;
} sink;

static size_t write_sink(void *user, const unsigned char *data, size_t size)
{
    sink *output = (sink *)user;
    size_t accepted = size < output->limit ? size : output->limit;
    if (accepted > sizeof(output->data) - output->used) {
        return 0;
    }
    memcpy(output->data + output->used, data, accepted);
    output->used += accepted;
    return accepted;
}

int main(void)
{
    sl_logger logger;
    sink output = {{0}, 0u, 2u};
    char oversized[SL_MAX_LINE_BYTES + 2u];
    CHECK(sl_init(NULL, write_sink, &output) == SL_INVALID_ARGUMENT);
    CHECK(sl_init(&logger, NULL, &output) == SL_INVALID_ARGUMENT);
    CHECK(sl_write_line(&logger, "{}") == SL_INVALID_ARGUMENT);
    CHECK(sl_init(&logger, write_sink, &output) == SL_OK);
    CHECK(sl_write_line(&logger, "{\"a\":1}") == SL_OK);
    CHECK(output.used == 8u);
    CHECK(memcmp(output.data, "{\"a\":1}\n", 8u) == 0);
    output.used = 0u;
    CHECK(sl_write_line(&logger, "a\nb") == SL_INVALID_ARGUMENT);
    CHECK(sl_write_line(&logger, "a\rb") == SL_INVALID_ARGUMENT);
    CHECK(sl_write_line(&logger, "") == SL_INVALID_ARGUMENT);
    CHECK(sl_write_line(&logger, NULL) == SL_INVALID_ARGUMENT);
    CHECK(output.used == 0u);
    memset(oversized, 'x', sizeof(oversized));
    oversized[sizeof(oversized) - 1u] = '\0';
    CHECK(sl_write_line(&logger, oversized) == SL_LINE_TOO_LONG);
    CHECK(output.used == 0u);
    oversized[SL_MAX_LINE_BYTES] = '\0';
    CHECK(sl_write_line(&logger, oversized) == SL_OK);
    CHECK(output.used == SL_MAX_LINE_BYTES + 1u);
    output.limit = 0u;
    CHECK(sl_write_line(&logger, "{}") == SL_WRITE_FAILED);
    return 0;
}
