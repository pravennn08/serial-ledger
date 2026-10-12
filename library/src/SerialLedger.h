#ifndef SERIALLEDGER_CPP_H
#define SERIALLEDGER_CPP_H
#include "serialledger_c.h"
#ifdef ARDUINO
#include <Arduino.h>
#endif

class SerialLedger {
public:
    SerialLedger(sl_write_fn write, void *user = nullptr) : logger_{} {
        sl_init(&logger_, write, user);
    }
#ifdef ARDUINO
    explicit SerialLedger(Print &output) : SerialLedger(&writeArduino, &output) {}
#endif
    sl_status writeLine(const char *line) { return sl_write_line(&logger_, line); }
    static const char *version() { return SL_VERSION; }
private:
    sl_logger logger_;
#ifdef ARDUINO
    static size_t writeArduino(void *user, const unsigned char *data, size_t size) {
        return static_cast<Print *>(user)->write(data, size);
    }
#endif
};
#endif
