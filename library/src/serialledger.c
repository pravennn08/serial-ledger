#include "serialledger_c.h"

static sl_status sl_write_all(sl_logger *logger, const unsigned char *data, size_t size)
{
    while (size > 0u) {
        const size_t written = logger->write(logger->user, data, size);
        if (written == 0u || written > size) {
            return SL_WRITE_FAILED;
        }
        data += written;
        size -= written;
    }
    return SL_OK;
}

sl_status sl_init(sl_logger *logger, sl_write_fn write, void *user)
{
    if (logger == NULL) {
        return SL_INVALID_ARGUMENT;
    }
    logger->write = write;
    logger->user = user;
    return write != NULL ? SL_OK : SL_INVALID_ARGUMENT;
}

sl_status sl_write_line(sl_logger *logger, const char *line)
{
    size_t length = 0u;
    sl_status status;
    static const unsigned char newline = '\n';
    if (logger == NULL || logger->write == NULL || line == NULL || line[0] == '\0') {
        return SL_INVALID_ARGUMENT;
    }
    while (line[length] != '\0') {
        if (length >= SL_MAX_LINE_BYTES) {
            return SL_LINE_TOO_LONG;
        }
        if (line[length] == '\r' || line[length] == '\n') {
            return SL_INVALID_ARGUMENT;
        }
        ++length;
    }
    status = sl_write_all(logger, (const unsigned char *)line, length);
    return status == SL_OK ? sl_write_all(logger, &newline, 1u) : status;
}
