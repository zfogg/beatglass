param([string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path)
$ErrorActionPreference = 'Stop'
function Assert-Path([string]$Path) { if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Missing qualification artifact: $Path" } }
$windows = Join-Path $Root 'tests/windows/run_qualification.ps1'
$macos = Join-Path $Root 'tests/macos/run_qualification.sh'
$macosVerify = Join-Path $Root 'tests/macos/verify_qualification_artifacts.sh'
Assert-Path $windows; Assert-Path $macos; Assert-Path $macosVerify
$tokens = $null; $errors = $null
[System.Management.Automation.Language.Parser]::ParseFile($windows, [ref]$tokens, [ref]$errors) | Out-Null
if ($errors.Count) { throw "Windows qualification script has $($errors.Count) parse error(s)" }
if (Get-Command bash -ErrorAction SilentlyContinue) {
  foreach ($script in @($macos, $macosVerify)) { & bash -n $script; if ($LASTEXITCODE -ne 0) { throw "Shell syntax check failed: $script" } }
} else { Write-Warning 'bash unavailable; shell syntax checks skipped' }
Write-Output 'qualification scripts: syntax and required-artifact checks passed'
