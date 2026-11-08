[CmdletBinding()]
param(
  [Parameter(Mandatory = $true, Position = 0)]
  [string]$OutputDirectory
)

$ErrorActionPreference = "Stop"
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path
$manifestPath = Join-Path $OutputDirectory "qualification-artifacts.sha256"
$evidencePath = Join-Path $OutputDirectory "native-playback-evidence.json"
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw "Missing artifact hash manifest: $manifestPath" }
if (-not (Test-Path -LiteralPath $evidencePath -PathType Leaf)) { throw "Missing native playback evidence template: $evidencePath" }
$lines = @(Get-Content -LiteralPath $manifestPath | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
if ($lines.Count -ne 4) { throw "Expected exactly four artifact hashes, found $($lines.Count)" }
$seen = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($line in $lines) {
  if ($line -notmatch '^(?<hash>[0-9a-fA-F]{64})\s{2}(?<relative>[^\s].*)$') { throw "Malformed artifact hash line: $line" }
  $expected = $Matches.hash
  $relative = $Matches.relative.Trim()
  if ([IO.Path]::IsPathRooted($relative)) { throw "Artifact path must be relative: $relative" }
  $candidate = [IO.Path]::GetFullPath((Join-Path $OutputDirectory $relative))
  $root = [IO.Path]::GetFullPath($OutputDirectory).TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
  if (-not $candidate.StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) { throw "Artifact path escapes output: $relative" }
  if (-not $seen.Add($candidate)) { throw "Duplicate artifact path: $relative" }
  if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) { throw "Missing artifact: $candidate" }
  $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $candidate).Hash
  if ($actual -ine $expected) { throw "Hash mismatch for $relative (expected $expected, got $actual)" }
}
$expectedCases = @("endpoint-selection-start", "pause-resume", "seek-epoch-retirement", "endpoint-disappearance-recovery", "retry-native-diagnostic", "stop-cleanup")
$evidence = @(Get-Content -LiteralPath $evidencePath -Raw | ConvertFrom-Json)
if ($evidence.Count -ne $expectedCases.Count) { throw "Expected exactly six native playback evidence cases" }
for ($i = 0; $i -lt $expectedCases.Count; $i++) {
  if ($evidence[$i].case -ne $expectedCases[$i]) { throw "Unexpected native playback evidence case: $($evidence[$i].case)" }
  if ($evidence[$i].status -notin @("PASS", "FAIL", "UNRUN")) { throw "Invalid status for $($evidence[$i].case): $($evidence[$i].status)" }
  if ($evidence[$i].status -eq "PASS" -and [string]::IsNullOrWhiteSpace($evidence[$i].log)) { throw "PASS case lacks log evidence: $($evidence[$i].case)" }
}
Write-Output "qualification artifact hashes verified: $OutputDirectory"
