# PipeWire initial discontinuity

Each newly allocated monitor stream marks its first callback block as a
discontinuity. This makes the start of every capture epoch explicit to
downstream analysis, including the initial acquisition and reacquisition.
