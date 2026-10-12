#include "SerialLedger.h"
#include <cstdio>
static size_t write_stdout(void *user, const unsigned char *data, size_t size) {
    return std::fwrite(data, 1u, size, static_cast<FILE *>(user));
}
int main() {
    SerialLedger logger(write_stdout, stdout);
    return logger.writeLine("{\"temperature_C\":25.4,\"humidity_pct\":61.2}") == SL_OK ? 0 : 1;
}
