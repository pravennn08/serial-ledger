#include "SerialLedger.h"
#include <cstring>
#include <string>
static size_t append(void *user, const unsigned char *data, size_t size) {
    static_cast<std::string *>(user)->append(reinterpret_cast<const char *>(data), size);
    return size;
}
int main() {
    std::string captured;
    SerialLedger logger(append, &captured);
    if (logger.writeLine("{\"a\":1}") != SL_OK || captured != "{\"a\":1}\n") {
        return 1;
    }
    if (std::strcmp(logger.version(), SL_VERSION) != 0) {
        return 1;
    }
    SerialLedger invalid(nullptr);
    return invalid.writeLine("{}") == SL_INVALID_ARGUMENT ? 0 : 1;
}
