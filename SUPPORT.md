# Support

SerialLedger is currently an initial development prototype. A public issue tracker
has not been configured yet.

For serial problems, record the board model, serial port, baud rate, firmware sample,
and the exact error from `uv run python -m serial_ledger record`. Close other serial
monitors and confirm the device appears in `uv run python -m serial_ledger ports`.

For setup problems, include `uv --version`, the Python version, and the output of
`scripts/setup.ps1`. Start with `uv run python -m serial_ledger demo` to distinguish host-side
problems from board or cable problems.
