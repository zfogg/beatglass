[CmdletBinding()]
param(
  [Parameter(Position = 0)]
  [string]$OutputDirectory = "",
  [switch]$Build
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
  Get-Command cmake | Select-Object -ExpandProperty Source
  cmake --version | Select-Object -First 1
  if ($env:VCPKG_ROOT) { "VCPKG_ROOT: $env:VCPKG_ROOT" } else { "VCPKG_ROOT: UNSET" }

  if ($Build) {
    New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
    $configureArgs = @(
      "-S", $repoRoot, "-B", $buildDirectory,
      "-G", "Visual Studio 17 2022", "-A", "x64",
      "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_TESTING=ON",
      "-DBEATGLASS_BUILD_DESKTOP=ON", "-DBEATGLASS_BUILD_WINDOWS_OVERLAY_TESTS=ON"
    )
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
  $ctestList = Join-Path $OutputDirectory "ctest-discovery.log"
  & ctest --test-dir $buildDirectory -C Release -N | Tee-Object -FilePath $ctestList
  if ($LASTEXITCODE -ne 0) { throw "CTest discovery failed with exit code $LASTEXITCODE" }
  $discovered = Get-Content $ctestList -Raw
  if ($discovered -notmatch "windows_endpoint_tests|windows_dj_bridge|windows_overlay") {
    throw "No Windows platform tests were discovered in $buildDirectory"
  }

  $required = @(
    (Join-Path $buildDirectory "Release/beatglass-desktop.exe"),
    (Join-Path $buildDirectory "Release/windows_endpoint_tests.exe")
  )
  foreach ($path in $required) {
    if (-not (Test-Path $path)) { throw "Required qualification artifact is missing: $path" }
  }
  "Prepared artifacts: $($required -join ', ')"
  "UNRUN: native WASAPI routing, endpoint removal/recovery, overlay, DPI, and interactive UI cases require a logged-in Windows session."
}
finally {
  Stop-Transcript | Out-Null
}
