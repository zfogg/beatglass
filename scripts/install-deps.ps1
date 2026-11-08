[CmdletBinding()]
param(
  [ValidateSet('x64', 'arm64')]
  [string] $Architecture = $(if ([Environment]::Is64BitOperatingSystem -and $env:PROCESSOR_ARCHITECTURE -match 'ARM') { 'arm64' } else { 'x64' }),
  [switch] $SkipVcpkg
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

function Install-WithWinget([string[]] $Ids) {
  if (-not (Get-Command winget -ErrorAction SilentlyContinue)) { return $false }
  foreach ($id in $Ids) {
    winget install --id $id --exact --accept-source-agreements --accept-package-agreements --scope user --silent --disable-interactivity
    if ($LASTEXITCODE -ne 0) { throw "winget failed installing $id (exit $LASTEXITCODE)" }
  }
  return $true
}

function Install-WithChoco([string[]] $Names) {
  if (-not (Get-Command choco -ErrorAction SilentlyContinue)) { return $false }
  choco install -y @Names
  if ($LASTEXITCODE -ne 0) { throw "Chocolatey failed installing prerequisites (exit $LASTEXITCODE)" }
  return $true
}

Write-Host "Installing Beatglass prerequisites for Windows $Architecture ($([Environment]::OSVersion.Version))"
$installed = Install-WithWinget @('Kitware.CMake', 'Git.Git', 'Ninja-build.Ninja', 'Python.Python.3.12')
if (-not $installed) {
  $installed = Install-WithChoco @('cmake', 'git', 'ninja', 'python')
}
if (-not $installed) {
  throw 'Install winget or Chocolatey, then rerun this script.'
}

if (-not $SkipVcpkg) {
  $vcpkgRoot = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { Join-Path $env:LOCALAPPDATA 'beatglass\vcpkg' }
  if (-not (Test-Path (Join-Path $vcpkgRoot 'vcpkg.exe'))) {
    if (Test-Path $vcpkgRoot) { throw "VCPKG_ROOT exists but is not a vcpkg checkout: $vcpkgRoot" }
    New-Item -ItemType Directory -Path (Split-Path $vcpkgRoot) -Force | Out-Null
    git clone --depth 1 https://github.com/microsoft/vcpkg.git $vcpkgRoot
    if ($LASTEXITCODE -ne 0) { throw 'Could not clone vcpkg' }
    & (Join-Path $vcpkgRoot 'bootstrap-vcpkg.bat') -disableMetrics
    if ($LASTEXITCODE -ne 0) { throw 'Could not bootstrap vcpkg' }
  }
  $triplet = "$Architecture-windows"
  & (Join-Path $vcpkgRoot 'vcpkg.exe') install --triplet $triplet aubio[core] libsndfile[core] sqlite3 qtdeclarative qtshadertools
  if ($LASTEXITCODE -ne 0) { throw 'vcpkg dependency installation failed' }
  Write-Host "Set VCPKG_ROOT=$vcpkgRoot before configuring with the vcpkg preset."
}

Write-Host "Beatglass prerequisites installed for $Architecture. Run cmake configure from the repository root."
