# Dependency-light library smokes

`run-library-smokes.ps1` compiles and runs a focused executor/analysis/storage
lifecycle matrix without SQLite, Catch2, Qt, or the aggregate test binary.
It covers submission, bounded capacity, priority/FIFO scheduling, progress,
cooperative cancellation, shutdown, terminal outcomes, and analysis retry /
verification behavior, serialization/callback failures, and storage
configuration validation. The compiler can be selected explicitly for a
toolchain or dependency environment. Each run writes `summary.txt` with
pass/fail counts in the selected output directory. It also writes
`commands.log` with the exact compiler invocations and elapsed time per case.

The output directory is optional and defaults to `build/standalone-smokes`:

```powershell
.\tests/standalone/run-library-smokes.ps1
tests/standalone/run-library-smokes.ps1 -OutputDir C:/temp/beatglass-smokes
tests/standalone/run-library-smokes.ps1 -Compiler C:/path/to/g++.exe
tests/standalone/run-library-smokes.ps1 -ContinueOnFailure
tests/standalone/run-library-smokes.ps1 -ListCases
tests/standalone/run-library-smokes.ps1 -SelectedCases analysis_progress,storage_config
tests/standalone/run-library-smokes.ps1 -OpenSSLRoot C:/path/to/openssl
```

The generated binaries are build artifacts and should not be committed.
