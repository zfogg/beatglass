[CmdletBinding()]
param([string]$OutputDir = "", [string]$Compiler = "g++", [switch]$ContinueOnFailure)

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
  @{ Name = "analysis_cancellation"; Sources = @($runnerSources + "tests/standalone/analysis_postverify_cancel_smoke.cpp") },
  @{ Name = "analysis_operation"; Sources = @("tests/standalone/analysis_operation_smoke.cpp") },
  @{ Name = "analysis_verifier_error"; Sources = @($runnerSources + "tests/standalone/analysis_verifier_error_smoke.cpp") },
  @{ Name = "analysis_empty_cancel"; Sources = @($runnerSources + "tests/standalone/analysis_empty_cancel_smoke.cpp") },
  @{ Name = "analysis_serialize_exception"; Sources = @($runnerSources + "tests/standalone/analysis_serialize_exception_smoke.cpp") },
  @{ Name = "storage_config"; Sources = @("tests/standalone/storage_config_smoke.cpp") },
  @{ Name = "analysis_postserialize_cancel"; Sources = @($runnerSources + "tests/standalone/analysis_postserialize_cancel_smoke.cpp") },
  @{ Name = "analysis_postverify_cancel"; Sources = @($runnerSources + "tests/standalone/analysis_postverify_cancel_smoke.cpp") },
  @{ Name = "analysis_empty_result"; Sources = @($runnerSources + "tests/standalone/analysis_empty_result_smoke.cpp") },
  @{ Name = "analysis_verifier_exception"; Sources = @($runnerSources + "tests/standalone/analysis_exception_smoke.cpp") },
  @{ Name = "analysis_progress_exception"; Sources = @($runnerSources + "tests/standalone/analysis_progress_exception_smoke.cpp") },
  @{ Name = "catalog"; Sources = @("lib/library/catalog.cpp", "tests/standalone/catalog_smoke.cpp") },
  @{ Name = "file_details"; Sources = @("lib/library/details.cpp", "tests/standalone/details_smoke.cpp") },
  @{ Name = "store_contract"; Sources = @("tests/standalone/store_contract_smoke.cpp") },
  @{ Name = "job_lifecycle"; Sources = @("tests/standalone/job_lifecycle_smoke.cpp") },
  @{ Name = "retryable_analysis"; Sources = @($runnerSources + "tests/standalone/retryable_analysis_smoke.cpp") }
)

$passed = [System.Collections.Generic.List[string]]::new()
$failed = [System.Collections.Generic.List[string]]::new()
Push-Location $repo
try {
  foreach ($case in $cases) {
    $binary = Join-Path $OutputDir "$($case.Name).exe"
    try {
      & $Compiler @common $case.Sources -o $binary
      if ($LASTEXITCODE -ne 0) { throw "compile failed" }
      & $binary
      if ($LASTEXITCODE -ne 0) { throw "smoke failed" }
      $passed.Add($case.Name)
      Write-Output "PASS $($case.Name)"
    } catch {
      $failed.Add($case.Name)
      Write-Error "FAIL $($case.Name): $_"
      if (-not $ContinueOnFailure) { throw }
    }
  }
} finally { Pop-Location }
$summary = "passed=$($passed.Count); failed=$($failed.Count); total=$($cases.Count)"
$summary | Set-Content -Path (Join-Path $OutputDir "summary.txt")
if ($failed.Count -gt 0) { throw "Standalone library smokes failed: $($failed -join ', ')" }
Write-Output "Standalone library smokes $summary"
