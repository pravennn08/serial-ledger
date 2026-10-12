#ifndef SERIALLEDGER_C_H
#define SERIALLEDGER_C_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

#define SL_VERSION "0.1.0"
#define SL_MAX_LINE_BYTES 4096u

/* Return bytes accepted; zero signals a transport failure. */
typedef size_t (*sl_write_fn)(void *user, const unsigned char *data, size_t size);
typedef struct {
    sl_write_fn write;
    void *user;
} sl_logger;
typedef enum {
    SL_OK = 0, SL_INVALID_ARGUMENT, SL_LINE_TOO_LONG, SL_WRITE_FAILED
} sl_status;

sl_status sl_init(sl_logger *logger, sl_write_fn write, void *user);
/* Sends a nonempty line plus LF, rejecting embedded CR/LF before writing.
 * The caller formats JSON. Calls are synchronous and must not run concurrently.
 * A partial transport failure may leave an incomplete record at the receiver.
 */
sl_status sl_write_line(sl_logger *logger, const char *line);

#ifdef __cplusplus
}
#endif
#endif
