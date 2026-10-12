# Architecture

Sensor application -> C transport core -> UART/Serial callback -> PC line framer ->
raw session CSV -> Excel export.

The C99 core has no allocation, Python, Arduino, or filesystem dependencies. A caller
supplies a synchronous write callback. Partial writes are retried; zero bytes written
signals failure. The C++11 wrapper delegates to the C API. Under Arduino it can write
to any `Print` object. A generic C UART adapter is supplied by the firmware application.

Public headers live in `library/src/`, alongside the core, to match Arduino's library
layout. Native examples stay at repository level; Arduino examples are packaged inside
`library/examples/`. CMake builds the same core with MSVC on the development PC.

The Python companion is managed with uv. pySerial receives bytes; bounded framing
keeps partial lines across read timeouts. Raw CSV logs preserve data without requiring
a fixed set of sensors. Export discovers JSON fields and writes typed XLSX columns.
The initial UI is a command line. Charts, sessions UI, typed firmware helpers, and
reconnection are planned features, not implemented promises.

The monorepo contains the desktop app and library. A future Arduino registry release
must package `library/` as the release repository root, with its own metadata, license,
documentation, and version tags. The desktop app will be distributed separately.

References: [Arduino library specification](https://docs.arduino.cc/arduino-cli/library-specification)
and [uv GitHub Actions integration](https://docs.astral.sh/uv/guides/integration/github/).
