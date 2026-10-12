# Contributing

Use uv for Python packages and keep `uv.lock` in version control. Use `uv add` for
runtime dependencies and `uv add --dev` for development tools. Never commit `.venv`,
`build`, `data`, or `work`.

Run `scripts/setup.ps1` on Windows before submitting changes. This runs the native
build, C/C++ tests, Python tests, and Ruff. If you change the wire format, update
`docs/PROTOCOL.md` and add a meaningful compatibility check.

CI runs the host checks on Windows/Linux and compiles the example for Arduino Uno
and ESP32. See `docs/CI.md`. Keep `uv.lock` and the workflow in your commits.

Keep the C core free of Arduino headers, dynamic allocation, and filesystem access.
Put platform integration in the C++ wrapper or firmware-supplied transport callbacks.
Do not claim board compatibility without compiling and testing on that board.

Public contributions and distribution should wait until the project license is selected.
