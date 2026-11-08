[CmdletBinding()]
param()
$ErrorActionPreference = "Stop"
$root = Join-Path $env:TEMP ("beatglass-verify-" + [guid]::NewGuid().ToString("N"))
try {
  $build = Join-Path $root "build/Release"
  New-Item -ItemType Directory -Force -Path $build | Out-Null
  $paths = @("beatglass-desktop.exe", "windows_endpoint_tests.exe", "windows_dj_bridge_tests.exe", "windows_overlay_tests.exe") | ForEach-Object { Join-Path $build $_ }
  foreach ($path in $paths) { Set-Content -LiteralPath $path -Value "fixture" -NoNewline }
  $hashes = foreach ($path in $paths) { $h = Get-FileHash -Algorithm SHA256 -LiteralPath $path; "{0}  {1}" -f $h.Hash.ToLowerInvariant(), ([IO.Path]::GetRelativePath($root, $path).Replace('\', '/')) }
  $hashes | Set-Content -LiteralPath (Join-Path $root "qualification-artifacts.sha256")
  @("commit=test", "captured_at=2026-01-01T00:00:00.0000000Z", "windows_version=test", "architecture=True", "cmake_version=test", "qt_version=UNAVAILABLE", "hardware_model=fixture", "artifact_sha256=qualification-artifacts.sha256", "playback_evidence=native-playback-evidence.json", "overlay_evidence=native-overlay-evidence.json", "display_topology=display-topology.json", "audio_inventory=audio-endpoints.json") | Set-Content -LiteralPath (Join-Path $root "qualification-manifest.txt")
  @{ captured_at = (Get-Date -AsUTC -Format o); monitors = @(@{ Name = "fixture" }); adapters = @(@{ Name = "fixture" }) } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root "display-topology.json")
  @{ captured_at = (Get-Date -AsUTC -Format o); endpoints = @(@{ Name = "fixture"; Status = "OK"; PNPDeviceID = "fixture" }) } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root "audio-endpoints.json")
  @(@{ case = "endpoint-selection-start"; status = "UNRUN" }, @{ case = "pause-resume"; status = "UNRUN" }, @{ case = "seek-epoch-retirement"; status = "UNRUN" }, @{ case = "endpoint-disappearance-recovery"; status = "UNRUN" }, @{ case = "retry-native-diagnostic"; status = "UNRUN" }, @{ case = "stop-cleanup"; status = "UNRUN" }) | ConvertTo-Json | Set-Content (Join-Path $root "native-playback-evidence.json")
  @(@{ case = "show-hide-recovery"; status = "UNRUN" }, @{ case = "click-through-unlock"; status = "UNRUN" }, @{ case = "display-change-clamp"; status = "UNRUN" }, @{ case = "dpi-fullscreen"; status = "UNRUN" }) | ConvertTo-Json | Set-Content (Join-Path $root "native-overlay-evidence.json")
  & (Join-Path $PSScriptRoot "verify_qualification_artifacts.ps1") $root
  Write-Output "Windows qualification verifier regression passed"
} finally { if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force } }
