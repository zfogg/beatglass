[CmdletBinding()]
param(
  [Parameter(Position = 0)]
  [string]$OutputDirectory = "",
  [switch]$Build,
  [switch]$RunTests
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
  $OutputDirectory = Join-Path $repoRoot "build/windows-qualification"
}
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$buildDirectory = Join-Path $OutputDirectory "build"
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

Start-Transcript -Path (Join-Path $OutputDirectory "qualification.log") -Force | Out-Null
try {
  "Beatglass Windows qualification evidence"
  Get-Date -AsUTC -Format o
  git -C $repoRoot rev-parse HEAD
  "Repository: $repoRoot"
  "Output directory: $OutputDirectory"
  "Windows: $([Environment]::OSVersion.VersionString)"
  "Architecture: $([Environment]::Is64BitOperatingSystem)"
  $topologyPath = Join-Path $OutputDirectory "display-topology.json"
  $topology = @{
    captured_at = (Get-Date -AsUTC -Format o)
    monitors = @(Get-CimInstance Win32_DesktopMonitor | Select-Object Name, MonitorType, ScreenHeight, ScreenWidth, PNPDeviceID)
    adapters = @(Get-CimInstance Win32_VideoController | Select-Object Name, DriverVersion, VideoModeDescription)
  }
  $topology | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $topologyPath -Encoding utf8
  if ((Get-Item -LiteralPath $topologyPath).Length -eq 0) { throw "Display topology evidence is empty" }
  "Display topology evidence: $topologyPath"
  $audioInventoryPath = Join-Path $OutputDirectory "audio-endpoints.json"
  $audioInventory = @{ captured_at = (Get-Date -AsUTC -Format o); endpoints = @(Get-CimInstance Win32_SoundDevice | Select-Object Name, Status, PNPDeviceID) }
  $audioInventory | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $audioInventoryPath -Encoding utf8
  if ((Get-Item -LiteralPath $audioInventoryPath).Length -eq 0) { throw "Audio endpoint inventory is empty" }
  "Audio endpoint inventory: $audioInventoryPath"
  Get-Command cmake | Select-Object -ExpandProperty Source
  cmake --version | Select-Object -First 1
  if ($env:VCPKG_ROOT) { "VCPKG_ROOT: $env:VCPKG_ROOT" } else { "VCPKG_ROOT: UNSET" }

  if ($Build) {
    New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
    $configureArgs = @(
      "-S", $repoRoot, "-B", $buildDirectory,
      "-G", "Visual Studio 17 2022", "-A", "x64",
      "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_TESTING=ON",
      "-DBEATGLASS_BUILD_SLICE=ON", "-DBEATGLASS_BUILD_DESKTOP=ON", "-DBEATGLASS_BUILD_WINDOWS_OVERLAY_TESTS=ON",
      "-DBEATGLASS_FETCH_SQLITE3=ON"
    )
    if ($env:VCPKG_ROOT) {
      $toolchain = Join-Path $env:VCPKG_ROOT "scripts/buildsystems/vcpkg.cmake"
      if (Test-Path -LiteralPath $toolchain) {
        $configureArgs += "-DCMAKE_TOOLCHAIN_FILE=$toolchain"
        $configureArgs += "-DVCPKG_MANIFEST_FEATURES=desktop;tui"
      } else {
        "VCPKG toolchain not found at $toolchain; continuing with host dependencies."
      }
    }
    "cmake $($configureArgs -join ' ')"
    & cmake @configureArgs
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed with exit code $LASTEXITCODE" }
    $buildArgs = @("--build", $buildDirectory, "--config", "Release", "--parallel", "1")
    "cmake $($buildArgs -join ' ')"
    & cmake @buildArgs
    if ($LASTEXITCODE -ne 0) { throw "CMake build failed with exit code $LASTEXITCODE" }
  }

  if (-not (Test-Path $buildDirectory)) {
    throw "Build directory is missing: $buildDirectory. Re-run with -Build or provide a prepared build directory."
  }
  if ($RunTests) {
    $ctestRun = Join-Path $OutputDirectory "ctest-run.log"
    $testArgs = @("--test-dir", $buildDirectory, "-C", "Release", "--output-on-failure")
    "ctest $($testArgs -join ' ')"
    & ctest @testArgs 2>&1 | Tee-Object -FilePath $ctestRun
    if ($LASTEXITCODE -ne 0) { throw "CTest execution failed with exit code $LASTEXITCODE" }
  } else {
    "UNRUN: CTest execution (pass -RunTests to execute the discovered Windows tests)."
  }
  $ctestList = Join-Path $OutputDirectory "ctest-discovery.log"
  & ctest --test-dir $buildDirectory -C Release -N | Tee-Object -FilePath $ctestList
  if ($LASTEXITCODE -ne 0) { throw "CTest discovery failed with exit code $LASTEXITCODE" }
  $discovered = Get-Content $ctestList -Raw
  if ($discovered -notmatch "windows_endpoint_tests|windows_dj_bridge|windows_overlay") {
    throw "No Windows platform tests were discovered in $buildDirectory"
  }

  $required = @(
    (Join-Path $buildDirectory "Release/beatglass-desktop.exe"),
    (Join-Path $buildDirectory "Release/windows_endpoint_tests.exe"),
    (Join-Path $buildDirectory "Release/windows_dj_bridge_tests.exe"),
    (Join-Path $buildDirectory "Release/windows_overlay_tests.exe")
  )
  foreach ($path in $required) {
    if (-not (Test-Path $path)) { throw "Required qualification artifact is missing: $path" }
  }
  "Prepared artifacts: $($required -join ', ')"
  $manifestPath = Join-Path $OutputDirectory "qualification-artifacts.sha256"
  $hashLines = foreach ($path in $required) {
    $hash = Get-FileHash -Algorithm SHA256 -LiteralPath $path
    $relativePath = [IO.Path]::GetRelativePath($OutputDirectory, $path).Replace('\', '/')
    "{0}  {1}" -f $hash.Hash.ToLowerInvariant(), $relativePath
  }
  $hashLines | Set-Content -LiteralPath $manifestPath -Encoding utf8
  "Artifact SHA-256 manifest: $manifestPath"
  $identityManifest = Join-Path $OutputDirectory "qualification-manifest.txt"
  $qtVersion = if (Get-Command qmake -ErrorAction SilentlyContinue) { (& qmake --version | Select-Object -Last 1).Trim() } else { "UNAVAILABLE" }
  $hardwareModel = (Get-CimInstance Win32_ComputerSystem | Select-Object -First 1 | ForEach-Object { "$($_.Manufacturer) $($_.Model)" }).Trim()
  @(
    "commit=$(git -C $repoRoot rev-parse HEAD)"
    "captured_at=$((Get-Date).ToUniversalTime().ToString('o'))"
    "windows_version=$([Environment]::OSVersion.VersionString)"
    "architecture=$([Environment]::Is64BitOperatingSystem)"
    "cmake_version=$((cmake --version | Select-Object -First 1).Trim())"
    "qt_version=$qtVersion"
    "hardware_model=$hardwareModel"
    "vcpkg_root=$($env:VCPKG_ROOT)"
    "artifact_sha256=qualification-artifacts.sha256"
    "playback_evidence=native-playback-evidence.json"
    "overlay_evidence=native-overlay-evidence.json"
    "display_topology=display-topology.json"
    "audio_inventory=audio-endpoints.json"
  ) | Set-Content -LiteralPath $identityManifest -Encoding utf8
  "Qualification identity manifest: $identityManifest"
  $evidencePath = Join-Path $OutputDirectory "native-playback-evidence.json"
  $evidence = @(
    @{ case = "endpoint-selection-start"; status = "UNRUN"; device = ""; observed = ""; log = ""; screenshot = "" },
    @{ case = "pause-resume"; status = "UNRUN"; device = ""; observed = ""; log = ""; screenshot = "" },
    @{ case = "seek-epoch-retirement"; status = "UNRUN"; device = ""; observed = ""; log = ""; screenshot = "" },
    @{ case = "endpoint-disappearance-recovery"; status = "UNRUN"; device = ""; observed = ""; log = ""; screenshot = "" },
    @{ case = "retry-native-diagnostic"; status = "UNRUN"; device = ""; observed = ""; log = ""; screenshot = "" },
    @{ case = "stop-cleanup"; status = "UNRUN"; device = ""; observed = ""; log = ""; screenshot = "" }
  )
  $evidence | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath $evidencePath -Encoding utf8
  "Native playback evidence template: $evidencePath"
  $overlayEvidencePath = Join-Path $OutputDirectory "native-overlay-evidence.json"
  @(
    @{ case = "show-hide-recovery"; status = "UNRUN"; log = ""; screenshot = "" },
    @{ case = "click-through-unlock"; status = "UNRUN"; log = ""; screenshot = "" },
    @{ case = "display-change-clamp"; status = "UNRUN"; log = ""; screenshot = "" },
    @{ case = "dpi-fullscreen"; status = "UNRUN"; log = ""; screenshot = "" }
  ) | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath $overlayEvidencePath -Encoding utf8
  "Native overlay evidence template: $overlayEvidencePath"
  "UNRUN: native WASAPI routing, endpoint removal/recovery, overlay, DPI, and interactive UI cases require a logged-in Windows session."
}
finally {
  Stop-Transcript | Out-Null
}
