# Typed readings in C and C++

The board library builds flat JSON in a fixed buffer. It does not allocate memory
or write to the transport until a complete reading is sent. `sl_write_line()` and
`writeLine()` remain available for manually formatted JSON and debug text.

## C++ / Arduino

```cpp
#include <SerialLedger.h>
SerialLedger logger(Serial);
sl_status sendSensorReading(float temperature, float humidity) {
    SerialLedgerReading reading;
    reading.addUInt("time_ms", millis());
    reading.addFloat("temperature_C", temperature, 2);
    reading.addFloat("humidity_pct", humidity, 2);
    return logger.writeReading(reading);
}
```

Call `Serial.begin(115200)` in setup, then call `sendSensorReading(t, h)` from your
loop after successful sensor reads. Check its return status before reporting success.
The default reading owns
a **192-byte buffer**, including its terminating NUL. For a different capacity,
use `SerialLedgerReadingBuffer<256> reading;`. Copying and moving are disabled
because the object's C state points into its own buffer. Call `reset()` to reuse
an object for the next sample.

## C99

```c
#include "serialledger_c.h"
sl_status send_sensor_reading(sl_logger *logger, uint32_t uptime_ms,
                              double temperature, double humidity)
{
    char buffer[192];
    sl_reading reading;
    sl_reading_init(&reading, buffer, sizeof(buffer));
    sl_reading_add_uint(&reading, "time_ms", uptime_ms);
    sl_reading_add_float(&reading, "temperature_C", temperature, 2u);
    sl_reading_add_float(&reading, "humidity_pct", humidity, 2u);
    return sl_reading_send(logger, &reading);
}
```

Initialize the logger once with `sl_init(&logger, uart_write, uart_context)`, using
your firmware's UART callback and context, then call this helper with measured values.
Keep the buffer alive, do not change it directly, and reinitialize the reading for
each new sample. Text inputs must not overlap this buffer.

## Field types

| C function | C++ method | JSON value |
| --- | --- | --- |
| `sl_reading_add_int` | `addInt` | Signed 64-bit integer |
| `sl_reading_add_uint` | `addUInt` | Unsigned 64-bit integer |
| `sl_reading_add_float` | `addFloat` | Fixed decimal number |
| `sl_reading_add_bool` | `addBool` | `true` or `false` |
| `sl_reading_add_string` | `addString` | Escaped UTF-8 string |
| `sl_reading_add_null` | `addNull` | `null` |

Keys must be nonempty, valid UTF-8 strings. Use unique keys: duplicate names are not
checked, and the desktop JSON parser keeps the last value. Strings are NUL-terminated;
embedded NUL bytes cannot be represented with this string API. Quotes, backslashes,
and control bytes are escaped. UTF-8 is validated before a field is added. There are
no nested object or array helpers.

Float formatting uses **0-6 decimal places**, with a default of 2 in C++. It accepts
finite values with an absolute magnitude up to **1,000,000,000**. It rounds half
away from zero for representable inputs and removes the sign of rounded zero.
Formatting uses the value actually stored in the floating-point type; additional
decimal places do not add sensor accuracy. It is independent of locale and printf's
float support. Use integer fields for larger whole numbers, or string fields for
identifiers whose digits must remain exact when viewed in spreadsheet software.

## Errors and sending

Every add call returns `sl_status`. The **first field error is sticky**: later calls
return that error, and sending the reading performs no transport writes. You can
check the final `writeReading()` or `sl_reading_send()` result even when you do not
check every add call separately. Previous buffer contents remain unchanged when an
add fails. Reset/reinitialize before starting a new reading.

| Status | Meaning |
| --- | --- |
| `SL_OK` | Field added, initialization succeeded, or full line accepted by transport |
| `SL_INVALID_ARGUMENT` | Invalid pointer/key/UTF-8, NaN, invalid precision, or empty reading |
| `SL_BUFFER_TOO_SMALL` | Reading does not fit in the buffer, including NUL |
| `SL_LINE_TOO_LONG` | Escaped JSON payload would exceed 4096 bytes |
| `SL_VALUE_OUT_OF_RANGE` | Float magnitude exceeds the supported range, including infinity |
| `SL_WRITE_FAILED` | Transport did not accept the full line |

In C++, `status()` returns the builder status, and `json()` returns a read-only view
of the JSON or a null pointer after a field error. The initial `{}` is inspectable
but cannot be sent as a typed reading. An empty reading is rejected.

The transport remains synchronous. Do not call it concurrently. A transport error
after a partial write may leave an incomplete line at the receiver; automatic retry
of an entire reading, acknowledgements, and delivery guarantees are not implemented.
