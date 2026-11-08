# Dependency-light library smokes

`run-library-smokes.ps1` compiles and runs a focused executor/analysis/storage
lifecycle matrix without SQLite, Catch2, Qt, or the aggregate test binary.
It covers submission, bounded capacity, priority/FIFO scheduling, progress,
cooperative cancellation, shutdown, terminal outcomes, and analysis retry /
verification behavior, serialization/callback failures, and storage
configuration validation. The compiler can be selected explicitly for a
toolchain or dependency environment.

The output directory is optional and defaults to `build/standalone-smokes`:

```powershell
.\tests/standalone/run-library-smokes.ps1
tests/standalone/run-library-smokes.ps1 -OutputDir C:/temp/beatglass-smokes
tests/standalone/run-library-smokes.ps1 -Compiler C:/path/to/g++.exe
tests/standalone/run-library-smokes.ps1 -ContinueOnFailure
```

The generated binaries are build artifacts and should not be committed.
