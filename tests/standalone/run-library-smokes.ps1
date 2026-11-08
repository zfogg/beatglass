[CmdletBinding()]
param([string]$OutputDir = "", [string]$Compiler = "g++", [string[]]$SelectedCases = @(), [string]$OpenSSLRoot = "", [string]$SQLiteRoot = "", [switch]$ListCases, [switch]$ContinueOnFailure, [switch]$RunExisting)

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
  @{ Name = "executor_queued_cancellation"; Sources = @($executorSource, "tests/standalone/job_executor_queued_cancel_smoke.cpp") },
  @{ Name = "executor_concurrent_submit"; Sources = @($executorSource, "tests/standalone/job_executor_concurrent_submit_smoke.cpp") },
  @{ Name = "executor_explicit_failure"; Sources = @($executorSource, "tests/standalone/job_executor_explicit_fail_smoke.cpp") },
  @{ Name = "executor_progress"; Sources = @($executorSource, "tests/standalone/job_executor_progress_smoke.cpp") },
  @{ Name = "executor_handle_readers"; Sources = @($executorSource, "tests/standalone/job_handle_readers_smoke.cpp") },
  @{ Name = "executor_after_shutdown"; Sources = @($executorSource, "tests/standalone/job_executor_after_shutdown_smoke.cpp") },
  @{ Name = "executor_empty"; Sources = @($executorSource, "tests/standalone/job_executor_empty_smoke.cpp") },
  @{ Name = "executor_exception"; Sources = @($executorSource, "tests/standalone/job_executor_exception_smoke.cpp") },
  @{ Name = "executor_explicit_cancel"; Sources = @($executorSource, "tests/standalone/job_executor_explicit_cancel_smoke.cpp") },
  @{ Name = "executor_handle_terminal"; Sources = @($executorSource, "tests/standalone/job_handle_terminal_smoke.cpp") },
  @{ Name = "executor_handle_concurrent_cancel"; Sources = @($executorSource, "tests/standalone/job_handle_concurrent_cancel_smoke.cpp") },
  @{ Name = "analysis_runner"; Sources = @($runnerSources + "tests/standalone/analysis_runner_smoke.cpp") },
  @{ Name = "analysis_batch"; Sources = @($runnerSources + "tests/standalone/analysis_batch_smoke.cpp") },
  @{ Name = "analysis_capacity"; Sources = @($runnerSources + "tests/standalone/analysis_capacity_smoke.cpp") },
  @{ Name = "analysis_direct_cancellation"; Sources = @($runnerSources + "tests/standalone/analysis_cancel_smoke.cpp") },
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
  @{ Name = "analysis_verifier"; Sources = @($runnerSources + "tests/standalone/analysis_verifier_smoke.cpp") },
  @{ Name = "analysis_final_progress_exception"; Sources = @($runnerSources + "tests/standalone/analysis_final_progress_exception_smoke.cpp") },
  @{ Name = "executor_fifo"; Sources = @($executorSource, "tests/standalone/job_executor_fifo_smoke.cpp") },
  @{ Name = "executor_invalid_state"; Sources = @($executorSource, "tests/standalone/job_executor_invalid_state_smoke.cpp") },
  @{ Name = "executor_shutdown_queued"; Sources = @($executorSource, "tests/standalone/job_executor_shutdown_queued_smoke.cpp") },
  @{ Name = "catalog"; Sources = @("lib/library/catalog.cpp", "tests/standalone/catalog_smoke.cpp") },
  @{ Name = "file_details"; Sources = @("lib/library/details.cpp", "tests/standalone/details_smoke.cpp") },
  @{ Name = "store_contract"; Sources = @("tests/standalone/store_contract_smoke.cpp") },
  @{ Name = "job_lifecycle"; Sources = @("tests/standalone/job_lifecycle_smoke.cpp") },
  @{ Name = "retryable_analysis"; Sources = @($runnerSources + "tests/standalone/retryable_analysis_smoke.cpp") }
)
if (-not [string]::IsNullOrWhiteSpace($OpenSSLRoot)) { $cases += @{ Name = "file_fingerprint"; Sources = @("lib/library/file_fingerprint.cpp", "tests/standalone/file_fingerprint_smoke.cpp"); Extra = @("-I$OpenSSLRoot/include", "-L$OpenSSLRoot/lib", "-lcrypto") } }
if (-not [string]::IsNullOrWhiteSpace($SQLiteRoot)) { $cases += @{ Name = "sqlite_cache_acceptance"; Sources = @("lib/library/sqlite_cache.cpp", "tests/standalone/sqlite_cache_acceptance_smoke.cpp"); Extra = @("-I$SQLiteRoot/include", "-L$SQLiteRoot/lib", "-lsqlite3") } }
if ($ListCases) { $cases | ForEach-Object { $_.Name }; exit 0 }
if ($SelectedCases.Count -gt 0) {
  $SelectedCases = @($SelectedCases | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
  $available = @($cases.Name)
  $unknown = @($SelectedCases | Where-Object { $available -notcontains $_ })
  if ($unknown.Count -gt 0) { throw "Unknown smoke case(s): $($unknown -join ', ')" }
  $cases = @($cases | Where-Object { $SelectedCases -contains $_.Name })
}
if ($cases.Count -eq 0) { throw "No smoke cases selected" }

$passed = [System.Collections.Generic.List[string]]::new()
$failed = [System.Collections.Generic.List[string]]::new()
$log = [System.Collections.Generic.List[string]]::new()
$records = [System.Collections.Generic.List[object]]::new()
$runStarted = [DateTime]::UtcNow
Push-Location $repo
try {
  foreach ($case in $cases) {
    $binary = Join-Path $OutputDir "$($case.Name).exe"
    try {
      $extra = if ($null -ne $case.Extra) { $case.Extra } else { @() }
      $started = [DateTime]::UtcNow
      if (-not $RunExisting) {
        $compilerArgs = @($common + $case.Sources + $extra + @("-o", $binary))
        $log.Add("$Compiler $($compilerArgs -join ' ')")
        & $Compiler @compilerArgs
        if ($LASTEXITCODE -ne 0) { throw "compile failed" }
      } elseif (-not (Test-Path $binary)) { throw "missing existing binary: $binary" }
      & $binary
      if ($LASTEXITCODE -ne 0) { throw "smoke failed" }
      $passed.Add($case.Name)
      $records.Add([pscustomobject]@{ name=$case.Name; status="passed"; sources=@($case.Sources); binary=$binary; seconds=([math]::Round(([DateTime]::UtcNow - $started).TotalSeconds, 2)) })
      Write-Output ("PASS {0} ({1:n2}s)" -f $case.Name, ([DateTime]::UtcNow - $started).TotalSeconds)
    } catch {
      $failed.Add($case.Name)
      $records.Add([pscustomobject]@{ name=$case.Name; status="failed"; sources=@($case.Sources); binary=$binary; seconds=([math]::Round(([DateTime]::UtcNow - $started).TotalSeconds, 2)); error=$_.Exception.Message })
      Write-Error "FAIL $($case.Name): $_"
      if (-not $ContinueOnFailure) { throw }
    }
  }
} finally { Pop-Location }
$selection = if ($SelectedCases.Count -gt 0) { $SelectedCases -join "," } else { "all" }
$summary = "passed=$($passed.Count); failed=$($failed.Count); total=$($cases.Count); selected=$selection"
$summary | Set-Content -Path (Join-Path $OutputDir "summary.txt")
$log | Set-Content -Path (Join-Path $OutputDir "commands.log")
$report = [pscustomobject]@{
  selected=$selection
  compiler=$Compiler
  platform=$([System.Environment]::OSVersion.VersionString)
  powershell=$($PSVersionTable.PSVersion.ToString())
  run_existing=[bool]$RunExisting
  continue_on_failure=[bool]$ContinueOnFailure
  output_directory=$OutputDir
  passed=$passed
  failed=$failed
  total_seconds=([math]::Round(([DateTime]::UtcNow - $runStarted).TotalSeconds, 2))
  cases=$records
}
$report | ConvertTo-Json -Depth 4 | Set-Content -Path (Join-Path $OutputDir "summary.json")
if ($failed.Count -gt 0) { throw "Standalone library smokes failed: $($failed -join ', ')" }
Write-Output "Standalone library smokes $summary"
