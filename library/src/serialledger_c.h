#ifndef SERIALLEDGER_C_H
#define SERIALLEDGER_C_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define SL_VERSION "0.1.0-beta.1"
#define SL_MAX_LINE_BYTES 4096u
#define SL_MAX_DECIMAL_PLACES 6u
#define SL_MAX_FLOAT_MAGNITUDE 1000000000.0

/* Return bytes accepted; zero signals a transport failure. */
typedef size_t (*sl_write_fn)(void *user, const unsigned char *data, size_t size);
typedef struct {
    sl_write_fn write;
    void *user;
} sl_logger;
typedef enum {
    SL_OK = 0, SL_INVALID_ARGUMENT, SL_LINE_TOO_LONG, SL_WRITE_FAILED,
    SL_BUFFER_TOO_SMALL, SL_VALUE_OUT_OF_RANGE
} sl_status;

sl_status sl_init(sl_logger *logger, sl_write_fn write, void *user);
/* Sends a nonempty line plus LF, rejecting embedded CR/LF before writing.
 * Use typed reading helpers below, or supply your own JSON/debug text.
 * Calls are synchronous and must not run concurrently.
 * A partial transport failure may leave an incomplete record at the receiver.
 */
sl_status sl_write_line(sl_logger *logger, const char *line);

/* Caller-owned UTF-8 JSON builder; no allocation and no transport writes until send.
 * Initialize before use. Keep the buffer alive and do not modify it directly.
 * A field error is sticky: reinitialize the reading before reuse after an error.
 * Field names must be nonempty and should be unique. Text must be valid UTF-8,
 * NUL-terminated, and must not overlap the reading buffer.
 */
typedef struct {
    char *buffer;
    size_t capacity;
    size_t length;
    size_t fields;
    sl_status status;
} sl_reading;

sl_status sl_reading_init(sl_reading *reading, char *buffer, size_t capacity);
sl_status sl_reading_add_int(sl_reading *reading, const char *key, int64_t value);
sl_status sl_reading_add_uint(sl_reading *reading, const char *key, uint64_t value);
/* Fixed decimal format, round half away from zero, independent of locale/printf.
 * Rejects nonfinite values, magnitudes over SL_MAX_FLOAT_MAGNITUDE, and precision
 * over SL_MAX_DECIMAL_PLACES. Rounded zero has no minus sign.
 */
sl_status sl_reading_add_float(sl_reading *reading, const char *key,
                             double value, unsigned int decimal_places);
sl_status sl_reading_add_bool(sl_reading *reading, const char *key, int value);
sl_status sl_reading_add_string(sl_reading *reading, const char *key, const char *value);
sl_status sl_reading_add_null(sl_reading *reading, const char *key);
/* Empty or failed readings are never sent. Transport errors are returned directly. */
sl_status sl_reading_send(sl_logger *logger, const sl_reading *reading);

#ifdef __cplusplus
}
#endif
#endif
