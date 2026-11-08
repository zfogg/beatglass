[CmdletBinding()]
param([string]$OutputDir = "")

$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
if ([string]::IsNullOrWhiteSpace($OutputDir)) { $OutputDir = Join-Path $repo "build/standalone-smokes" }
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$common = @("-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-pthread", "-Iinclude")
$executorSource = "lib/library/job_executor.cpp"
$runnerSources = @("lib/library/job_executor.cpp", "lib/library/analysis_runner.cpp")
$cases = @(
  @{ Name = "executor_lifecycle"; Sources = @($executorSource, "tests/standalone/job_executor_smoke.cpp") },
  @{ Name = "executor_capacity"; Sources = @($executorSource, "tests/standalone/job_executor_capacity_smoke.cpp") },
  @{ Name = "executor_priority"; Sources = @($executorSource, "tests/standalone/job_executor_priority_smoke.cpp") },
  @{ Name = "executor_cancellation"; Sources = @($executorSource, "tests/standalone/job_executor_cancel_smoke.cpp") },
  @{ Name = "executor_shutdown"; Sources = @($executorSource, "tests/standalone/job_executor_shutdown_smoke.cpp") },
  @{ Name = "analysis_runner"; Sources = @($runnerSources + "tests/standalone/analysis_runner_smoke.cpp") },
  @{ Name = "analysis_progress"; Sources = @($runnerSources + "tests/standalone/analysis_progress_smoke.cpp") },
  @{ Name = "analysis_cancellation"; Sources = @($runnerSources + "tests/standalone/analysis_postverify_cancel_smoke.cpp") }
)

Push-Location $repo
try {
  foreach ($case in $cases) {
    $binary = Join-Path $OutputDir "$($case.Name).exe"
    & g++ @common $case.Sources -o $binary
    if ($LASTEXITCODE -ne 0) { throw "compile failed: $($case.Name)" }
    & $binary
    if ($LASTEXITCODE -ne 0) { throw "smoke failed: $($case.Name)" }
  }
} finally { Pop-Location }
Write-Output "Standalone library smokes passed: $($cases.Count)"
