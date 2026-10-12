#include "SerialLedger.h"
#include <cstring>
#include <string>
#include <type_traits>
static_assert(!std::is_copy_constructible<SerialLedgerReading>::value, "Own-buffer readers cannot copy");
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
    if (invalid.writeLine("{}") != SL_INVALID_ARGUMENT) {
        return 1;
    }
    captured.clear();
    SerialLedgerReading reading;
    reading.addUInt("seq", 1);
    reading.addFloat("temperature_C", 25.4);
    reading.addString("label", "a\"b");
    reading.addBool("enabled", true);
    reading.addNull("missing");
    if (logger.writeReading(reading) != SL_OK || captured !=
        "{\"seq\":1,\"temperature_C\":25.40,\"label\":\"a\\\"b\",\"enabled\":true,\"missing\":null}\n") {
        return 1;
    }
    SerialLedgerReadingBuffer<8> tiny;
    tiny.addInt("a", -1);
    captured.clear();
    if (tiny.status() != SL_BUFFER_TOO_SMALL || tiny.json() != nullptr ||
        logger.writeReading(tiny) != SL_BUFFER_TOO_SMALL || !captured.empty()) {
        return 1;
    }
    tiny.reset();
    tiny.addInt("a", 0);
    return logger.writeReading(tiny) == SL_OK && captured == "{\"a\":0}\n" ? 0 : 1;
}
