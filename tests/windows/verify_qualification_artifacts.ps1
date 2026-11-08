[CmdletBinding()]
param(
  [Parameter(Mandatory = $true, Position = 0)]
  [string]$OutputDirectory
)

$ErrorActionPreference = "Stop"
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path
$manifestPath = Join-Path $OutputDirectory "qualification-artifacts.sha256"
$evidencePath = Join-Path $OutputDirectory "native-playback-evidence.json"
$overlayEvidencePath = Join-Path $OutputDirectory "native-overlay-evidence.json"
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw "Missing artifact hash manifest: $manifestPath" }
if (-not (Test-Path -LiteralPath $evidencePath -PathType Leaf)) { throw "Missing native playback evidence template: $evidencePath" }
if (-not (Test-Path -LiteralPath $overlayEvidencePath -PathType Leaf)) { throw "Missing native overlay evidence template: $overlayEvidencePath" }
$lines = @(Get-Content -LiteralPath $manifestPath | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
if ($lines.Count -ne 4) { throw "Expected exactly four artifact hashes, found $($lines.Count)" }
$seen = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$relativePaths = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($line in $lines) {
  if ($line -notmatch '^(?<hash>[0-9a-fA-F]{64})\s{2}(?<relative>[^\s].*)$') { throw "Malformed artifact hash line: $line" }
  $expected = $Matches.hash
  $relative = $Matches.relative.Trim()
  [void]$relativePaths.Add($relative.Replace('\', '/'))
  if ([IO.Path]::IsPathRooted($relative)) { throw "Artifact path must be relative: $relative" }
  $candidate = [IO.Path]::GetFullPath((Join-Path $OutputDirectory $relative))
  $root = [IO.Path]::GetFullPath($OutputDirectory).TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
  if (-not $candidate.StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) { throw "Artifact path escapes output: $relative" }
  if (-not $seen.Add($candidate)) { throw "Duplicate artifact path: $relative" }
  if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) { throw "Missing artifact: $candidate" }
  if ((Get-Item -LiteralPath $candidate).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Artifact must not be a reparse point: $relative" }
  $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $candidate).Hash
  if ($actual -ine $expected) { throw "Hash mismatch for $relative (expected $expected, got $actual)" }
}
$expectedArtifacts = @("build/Release/beatglass-desktop.exe", "build/Release/windows_endpoint_tests.exe", "build/Release/windows_dj_bridge_tests.exe", "build/Release/windows_overlay_tests.exe")
foreach ($expected in $expectedArtifacts) { if (-not $relativePaths.Contains($expected)) { throw "Artifact manifest is missing required executable: $expected" } }
$expectedCases = @("endpoint-selection-start", "pause-resume", "seek-epoch-retirement", "endpoint-disappearance-recovery", "retry-native-diagnostic", "stop-cleanup")
$evidence = @(Get-Content -LiteralPath $evidencePath -Raw | ConvertFrom-Json)
if ($evidence.Count -ne $expectedCases.Count) { throw "Expected exactly six native playback evidence cases" }
for ($i = 0; $i -lt $expectedCases.Count; $i++) {
  if ($evidence[$i].case -ne $expectedCases[$i]) { throw "Unexpected native playback evidence case: $($evidence[$i].case)" }
  if ($evidence[$i].status -notin @("PASS", "FAIL", "UNRUN")) { throw "Invalid status for $($evidence[$i].case): $($evidence[$i].status)" }
  if ($evidence[$i].status -eq "PASS") {
    if ([string]::IsNullOrWhiteSpace($evidence[$i].log) -or [IO.Path]::IsPathRooted($evidence[$i].log)) { throw "PASS case lacks relative log evidence: $($evidence[$i].case)" }
    $logPath = [IO.Path]::GetFullPath((Join-Path $OutputDirectory $evidence[$i].log))
    $root = [IO.Path]::GetFullPath($OutputDirectory).TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    if (-not $logPath.StartsWith($root, [StringComparison]::OrdinalIgnoreCase) -or -not (Test-Path -LiteralPath $logPath -PathType Leaf)) { throw "Missing log evidence for $($evidence[$i].case): $($evidence[$i].log)" }
    if ((Get-Item -LiteralPath $logPath).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Log evidence must not be a reparse point: $($evidence[$i].log)" }
  }
}
$expectedOverlayCases = @("show-hide-recovery", "click-through-unlock", "display-change-clamp", "dpi-fullscreen")
$overlayEvidence = @(Get-Content -LiteralPath $overlayEvidencePath -Raw | ConvertFrom-Json)
if ($overlayEvidence.Count -ne $expectedOverlayCases.Count) { throw "Expected exactly four native overlay evidence cases" }
for ($i = 0; $i -lt $expectedOverlayCases.Count; $i++) {
  if ($overlayEvidence[$i].case -ne $expectedOverlayCases[$i]) { throw "Unexpected native overlay evidence case: $($overlayEvidence[$i].case)" }
  if ($overlayEvidence[$i].status -notin @("PASS", "FAIL", "UNRUN")) { throw "Invalid overlay status for $($overlayEvidence[$i].case): $($overlayEvidence[$i].status)" }
  if ($overlayEvidence[$i].status -eq "PASS") {
    if ([string]::IsNullOrWhiteSpace($overlayEvidence[$i].screenshot) -or [IO.Path]::IsPathRooted($overlayEvidence[$i].screenshot)) { throw "PASS overlay case lacks relative screenshot evidence: $($overlayEvidence[$i].case)" }
    $screenshotPath = [IO.Path]::GetFullPath((Join-Path $OutputDirectory $overlayEvidence[$i].screenshot))
    $root = [IO.Path]::GetFullPath($OutputDirectory).TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    if (-not $screenshotPath.StartsWith($root, [StringComparison]::OrdinalIgnoreCase) -or -not (Test-Path -LiteralPath $screenshotPath -PathType Leaf)) { throw "Missing screenshot evidence for $($overlayEvidence[$i].case): $($overlayEvidence[$i].screenshot)" }
    if ((Get-Item -LiteralPath $screenshotPath).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Screenshot evidence must not be a reparse point: $($overlayEvidence[$i].screenshot)" }
  }
}
Write-Output "qualification artifact hashes verified: $OutputDirectory"
