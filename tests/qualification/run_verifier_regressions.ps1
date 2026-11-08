[CmdletBinding()]
param()
$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
Write-Output "Running Windows qualification verifier regression"
& pwsh -NoProfile -File (Join-Path $repoRoot "tests/windows/verify_qualification_artifacts_test.ps1")
if (Get-Command bash -ErrorAction SilentlyContinue) {
  Write-Output "Running macOS qualification verifier regression under bash"
  & bash (Join-Path $repoRoot "tests/macos/verify_qualification_artifacts_test.sh")
} else {
  Write-Output "SKIP: bash unavailable; macOS verifier regression requires a Mac or bash"
}
Write-Output "Qualification verifier regressions complete"
