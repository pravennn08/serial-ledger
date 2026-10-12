#include "serialledger_c.h"
#include <string.h>

static sl_status sl_reading_error(sl_reading *reading, sl_status status)
{
    if (reading == NULL) {
        return SL_INVALID_ARGUMENT;
    }
    if (reading->status == SL_OK) {
        reading->status = status;
    }
    return reading->status;
}

/* Validate one codepoint without reading beyond a string's terminating NUL. */
static size_t sl_utf8_width(const unsigned char *text)
{
    size_t width;
    size_t index;
    uint32_t codepoint;
    uint32_t minimum;
    if (*text < 0x80u) {
        return 1u;
    }
    if (*text >= 0xc2u && *text <= 0xdfu) {
        width = 2u;
        codepoint = *text & 0x1fu;
        minimum = 0x80u;
    } else if (*text >= 0xe0u && *text <= 0xefu) {
        width = 3u;
        codepoint = *text & 0x0fu;
        minimum = 0x800u;
    } else if (*text >= 0xf0u && *text <= 0xf4u) {
        width = 4u;
        codepoint = *text & 0x07u;
        minimum = 0x10000u;
    } else {
        return 0u;
    }
    for (index = 1u; index < width; ++index) {
        if ((text[index] & 0xc0u) != 0x80u) {
            return 0u;
        }
        codepoint = (codepoint << 6u) | (text[index] & 0x3fu);
    }
    if (codepoint < minimum || codepoint > 0x10ffffu ||
        (codepoint >= 0xd800u && codepoint <= 0xdfffu)) {
        return 0u;
    }
    return width;
}

static sl_status sl_string_size(const char *text, size_t *size)
{
    const unsigned char *cursor = (const unsigned char *)text;
    size_t length = 2u; /* Quotes. */
    if (text == NULL) {
        return SL_INVALID_ARGUMENT;
    }
    while (*cursor != 0u) {
        const size_t width = sl_utf8_width(cursor);
        size_t escaped = width;
        if (width == 0u) {
            return SL_INVALID_ARGUMENT;
        }
        if (*cursor == '"' || *cursor == '\\') {
            escaped = 2u;
        } else if (*cursor < 0x20u) {
            escaped = 6u; /* \u00xx, including CR/LF. */
        }
        if (escaped > SL_MAX_LINE_BYTES - length) {
            return SL_LINE_TOO_LONG;
        }
        length += escaped;
        cursor += width;
    }
    *size = length;
    return SL_OK;
}

static char *sl_write_string(char *destination, const char *text)
{
    const unsigned char *cursor = (const unsigned char *)text;
    static const char hex[] = "0123456789abcdef";
    *destination++ = '"';
    while (*cursor != 0u) {
        if (*cursor == '"' || *cursor == '\\') {
            *destination++ = '\\';
            *destination++ = (char)*cursor;
        } else if (*cursor < 0x20u) {
            *destination++ = '\\';
            *destination++ = 'u';
            *destination++ = '0';
            *destination++ = '0';
            *destination++ = hex[*cursor >> 4u];
            *destination++ = hex[*cursor & 0x0fu];
        } else {
            *destination++ = (char)*cursor;
        }
        ++cursor;
    }
    *destination++ = '"';
    return destination;
}

static int sl_overlaps_buffer(const sl_reading *reading, const char *text)
{
    const uintptr_t start = (uintptr_t)reading->buffer;
    const uintptr_t address = (uintptr_t)text;
    if (address >= start) {
        return address - start < reading->capacity;
    }
    return start - address <= strlen(text);
}

static sl_status sl_add_value(sl_reading *reading, const char *key,
                              const char *value, int quote_value)
{
    size_t key_size;
    size_t value_size;
    size_t length;
    char *cursor;
    sl_status status;
    if (reading == NULL) {
        return SL_INVALID_ARGUMENT;
    }
    if (reading->status != SL_OK) {
        return reading->status;
    }
    if (key == NULL || *key == '\0' || value == NULL) {
        return sl_reading_error(reading, SL_INVALID_ARGUMENT);
    }
    status = sl_string_size(key, &key_size);
    if (status != SL_OK) {
        return sl_reading_error(reading, status);
    }
    if (quote_value) {
        status = sl_string_size(value, &value_size);
        if (status != SL_OK) {
            return sl_reading_error(reading, status);
        }
    } else {
        value_size = strlen(value); /* Only internally formatted scalars. */
    }
    if (sl_overlaps_buffer(reading, key) ||
        (quote_value && sl_overlaps_buffer(reading, value))) {
        return sl_reading_error(reading, SL_INVALID_ARGUMENT);
    }
    length = reading->length + key_size + 1u + value_size;
    if (reading->fields > 0u) {
        ++length;
    }
    if (length > SL_MAX_LINE_BYTES) {
        return sl_reading_error(reading, SL_LINE_TOO_LONG);
    }
    if (length >= reading->capacity) {
        return sl_reading_error(reading, SL_BUFFER_TOO_SMALL);
    }
    /* Preflight succeeded: replace the closing brace, then restore it. */
    cursor = reading->buffer + reading->length - 1u;
    if (reading->fields > 0u) {
        *cursor++ = ',';
    }
    cursor = sl_write_string(cursor, key);
    *cursor++ = ':';
    if (quote_value) {
        cursor = sl_write_string(cursor, value);
    } else {
        memcpy(cursor, value, value_size);
        cursor += value_size;
    }
    *cursor++ = '}';
    *cursor = '\0';
    reading->length = length;
    ++reading->fields;
    return SL_OK;
}

static char *sl_format_uint(char *destination, uint64_t value)
{
    char reverse[20];
    size_t count = 0u;
    do {
        reverse[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value != 0u);
    while (count > 0u) {
        *destination++ = reverse[--count];
    }
    return destination;
}

sl_status sl_reading_init(sl_reading *reading, char *buffer, size_t capacity)
{
    if (reading == NULL) {
        return SL_INVALID_ARGUMENT;
    }
    reading->buffer = buffer;
    reading->capacity = capacity;
    reading->length = 0u;
    reading->fields = 0u;
    reading->status = buffer == NULL ? SL_INVALID_ARGUMENT :
        (capacity < 3u ? SL_BUFFER_TOO_SMALL : SL_OK);
    if (reading->status == SL_OK) {
        memcpy(buffer, "{}", 3u);
        reading->length = 2u;
    }
    return reading->status;
}

sl_status sl_reading_add_int(sl_reading *reading, const char *key, int64_t value)
{
    char text[22];
    char *cursor = text;
    const uint64_t magnitude = value < 0 ? (uint64_t)(-(value + 1)) + 1u : (uint64_t)value;
    if (value < 0) {
        *cursor++ = '-';
    }
    cursor = sl_format_uint(cursor, magnitude);
    *cursor = '\0';
    return sl_add_value(reading, key, text, 0);
}

sl_status sl_reading_add_uint(sl_reading *reading, const char *key, uint64_t value)
{
    char text[21];
    char *end = sl_format_uint(text, value);
    *end = '\0';
    return sl_add_value(reading, key, text, 0);
}

sl_status sl_reading_add_float(sl_reading *reading, const char *key,
                             double value, unsigned int decimal_places)
{
    char text[32];
    char *cursor = text;
    const double magnitude = value < 0.0 ? -value : value;
    uint32_t scale = 1u;
    uint32_t whole;
    uint32_t fraction;
    uint32_t divisor;
    unsigned int index;
    if (decimal_places > SL_MAX_DECIMAL_PLACES || value != value) {
        return sl_reading_error(reading, SL_INVALID_ARGUMENT);
    }
    /* This also rejects infinity, before any floating-to-integer conversion. */
    if (magnitude > SL_MAX_FLOAT_MAGNITUDE) {
        return sl_reading_error(reading, SL_VALUE_OUT_OF_RANGE);
    }
    for (index = 0u; index < decimal_places; ++index) {
        scale *= 10u;
    }
    whole = (uint32_t)magnitude;
    fraction = (uint32_t)((magnitude - (double)whole) * (double)scale + 0.5);
    if (fraction >= scale) {
        ++whole;
        fraction = 0u;
    }
    if (value < 0.0 && (whole != 0u || fraction != 0u)) {
        *cursor++ = '-';
    }
    cursor = sl_format_uint(cursor, whole);
    if (decimal_places > 0u) {
        *cursor++ = '.';
        for (divisor = scale / 10u; divisor > 0u; divisor /= 10u) {
            *cursor++ = (char)('0' + fraction / divisor);
            fraction %= divisor;
        }
    }
    *cursor = '\0';
    return sl_add_value(reading, key, text, 0);
}

sl_status sl_reading_add_bool(sl_reading *reading, const char *key, int value)
{
    return sl_add_value(reading, key, value ? "true" : "false", 0);
}

sl_status sl_reading_add_string(sl_reading *reading, const char *key, const char *value)
{
    return sl_add_value(reading, key, value, 1);
}

sl_status sl_reading_add_null(sl_reading *reading, const char *key)
{
    return sl_add_value(reading, key, "null", 0);
}

sl_status sl_reading_send(sl_logger *logger, const sl_reading *reading)
{
    if (reading == NULL) {
        return SL_INVALID_ARGUMENT;
    }
    if (reading->status != SL_OK) {
        return reading->status;
    }
    if (reading->fields == 0u) {
        return SL_INVALID_ARGUMENT;
    }
    return sl_write_line(logger, reading->buffer);
}
