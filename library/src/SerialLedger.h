#ifndef SERIALLEDGER_CPP_H
#define SERIALLEDGER_CPP_H
#include "serialledger_c.h"
#ifdef ARDUINO
#include <Arduino.h>
#endif

template <size_t Capacity>
class SerialLedgerReadingBuffer {
public:
    static_assert(Capacity >= 3u, "A reading buffer must hold an empty JSON object and NUL");
    SerialLedgerReadingBuffer() { reset(); }
    SerialLedgerReadingBuffer(const SerialLedgerReadingBuffer &) = delete;
    SerialLedgerReadingBuffer &operator=(const SerialLedgerReadingBuffer &) = delete;
    sl_status reset() { return sl_reading_init(&reading_, buffer_, Capacity); }
    sl_status addInt(const char *key, int64_t value) {
        return sl_reading_add_int(&reading_, key, value);
    }
    sl_status addUInt(const char *key, uint64_t value) {
        return sl_reading_add_uint(&reading_, key, value);
    }
    sl_status addFloat(const char *key, double value, unsigned int decimalPlaces = 2u) {
        return sl_reading_add_float(&reading_, key, value, decimalPlaces);
    }
    sl_status addBool(const char *key, bool value) {
        return sl_reading_add_bool(&reading_, key, value);
    }
    sl_status addString(const char *key, const char *value) {
        return sl_reading_add_string(&reading_, key, value);
    }
    sl_status addNull(const char *key) { return sl_reading_add_null(&reading_, key); }
    sl_status status() const { return reading_.status; }
    const char *json() const { return reading_.status == SL_OK ? buffer_ : nullptr; }
    const sl_reading *native() const { return &reading_; }
private:
    char buffer_[Capacity];
    sl_reading reading_;
};

typedef SerialLedgerReadingBuffer<192u> SerialLedgerReading;

class SerialLedger {
public:
    SerialLedger(sl_write_fn write, void *user = nullptr) : logger_{} {
        sl_init(&logger_, write, user);
    }
#ifdef ARDUINO
    explicit SerialLedger(Print &output) : SerialLedger(&writeArduino, &output) {}
#endif
    sl_status writeLine(const char *line) { return sl_write_line(&logger_, line); }
    template <size_t Capacity>
    sl_status writeReading(const SerialLedgerReadingBuffer<Capacity> &reading) {
        return sl_reading_send(&logger_, reading.native());
    }
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
