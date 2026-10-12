#include "serialledger_c.h"
#include <stdio.h>
static size_t write_stdout(void *user, const unsigned char *data, size_t size)
{
    return fwrite(data, 1u, size, (FILE *)user);
}
int main(void)
{
    sl_logger logger;
    sl_reading reading;
    char buffer[192];
    if (sl_init(&logger, write_stdout, stdout) != SL_OK) {
        return 1;
    }
    sl_reading_init(&reading, buffer, sizeof(buffer));
    sl_reading_add_float(&reading, "temperature_C", 25.4, 2u);
    sl_reading_add_float(&reading, "humidity_pct", 61.2, 2u);
    return sl_reading_send(&logger, &reading) == SL_OK ? 0 : 1;
}
