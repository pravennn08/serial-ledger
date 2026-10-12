# Initial wire format (development)

Transport: serial/UART, normally 115200 baud, 8 data bits, no parity, one stop bit.
Encoding: UTF-8. Framing: one line terminated by LF; receiver also accepts CRLF.
Maximum board-side payload: 4096 bytes, excluding the line terminator.

Each telemetry record is one nonempty JSON object containing scalar values:

```json
{"time_ms":1000,"temperature_C":25.4,"humidity_pct":61.2}
```

Numbers must be finite. Strings, booleans, and null are supported. Nested objects and
arrays are retained as raw messages rather than expanded into spreadsheet columns.
Units are currently encoded in field names. A board may add `seq` and `time_ms`, but
sequence validation, clock synchronization, and schema messages are not implemented.

The typed C/C++ reading builder generates this flat JSON format and checks field
types, text encoding, buffer capacity, and number ranges. Its float output uses fixed
decimal places and never sends NaN or infinity. See [the API guide](BOARD_API.md).
The manual line transport checks framing and length, not JSON syntax. The PC parser recognizes
flat JSON readings and records other lines as `raw`. Invalid UTF-8 is decoded using
replacement characters and marked raw. Oversized incoming lines are discarded until
LF, and the recorder reports a dropped count. Incomplete lines remain buffered across
read timeouts; an unfinished final line is not recorded.

Session CSV columns: `received_at` (UTC ISO 8601), `parse_status` (`ok` or `raw`), `raw`.
Excel export adds `value.<field>` columns in first-seen order. Numerical readings are
numeric cells. Device text is explicitly stored as text, never Excel formulas.

Only one writer may use a logger at a time. A transport failure after a partial write
can leave an incomplete line at the receiver; the core does not promise delivery,
reconnection, acknowledgements, or nonblocking execution.
