# Dependency-light library smokes

`run-library-smokes.ps1` compiles and runs a focused executor/analysis
lifecycle matrix without SQLite, Catch2, Qt, or the aggregate test binary.
It covers submission, bounded capacity, priority/FIFO scheduling, progress,
cooperative cancellation, shutdown, terminal outcomes, and analysis retry /
verification behavior.

The output directory is optional and defaults to `build/standalone-smokes`:

```powershell
.\tests/standalone/run-library-smokes.ps1
tests/standalone/run-library-smokes.ps1 -OutputDir C:/temp/beatglass-smokes
```

The generated binaries are build artifacts and should not be committed.
