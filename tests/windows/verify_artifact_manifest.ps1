[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$Manifest)
$ErrorActionPreference = 'Stop'
$manifestPath = (Resolve-Path -LiteralPath $Manifest).Path
$base = Split-Path -Parent $manifestPath
$basePrefix = $base.TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
$lines = @(Get-Content -LiteralPath $manifestPath | Where-Object { $_.Trim() })
if ($lines.Count -eq 0) { throw "Artifact manifest is empty: $manifestPath" }
$seen = @{}
foreach ($line in $lines) {
  if ($line -notmatch '^([0-9a-fA-F]{64})  (.+)$') { throw "Malformed artifact manifest line: $line" }
  $expected = $Matches[1].ToLowerInvariant(); $relative = $Matches[2]
  if ($seen.ContainsKey($relative)) { throw "Duplicate artifact path: $relative" }
  $seen[$relative] = $true
  $path = Join-Path $base $relative
  if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing artifact: $path" }
  $resolved = (Resolve-Path -LiteralPath $path).Path
  if (-not $resolved.StartsWith($basePrefix, [StringComparison]::OrdinalIgnoreCase)) { throw "Artifact path escapes manifest directory: $relative" }
  $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $resolved).Hash.ToLowerInvariant()
  if ($actual -ne $expected) { throw "Hash mismatch: $path" }
}
Write-Output "Verified $($lines.Count) artifact hash(es) from $manifestPath"
