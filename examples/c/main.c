#include "serialledger_c.h"
#include <stdio.h>
static size_t write_stdout(void *user, const unsigned char *data, size_t size)
{
    return fwrite(data, 1u, size, (FILE *)user);
}
int main(void)
{
    sl_logger logger;
    if (sl_init(&logger, write_stdout, stdout) != SL_OK) {
        return 1;
    }
    return sl_write_line(&logger, "{\"temperature_C\":25.4,\"humidity_pct\":61.2}") == SL_OK ? 0 : 1;
}
