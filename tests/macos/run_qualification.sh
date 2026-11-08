#!/bin/bash
set -euo pipefail

# Run from the repository root on the logged-in Mac. This script does not grant
# permissions, start playback, or claim native qualification by itself.
repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
out_dir="${1:-$repo_root/build/macos-qualification}"
mkdir -p "$out_dir"
exec > >(tee "$out_dir/qualification.log") 2>&1

echo "Beatglass macOS qualification evidence"
date -u +%FT%TZ
git -C "$repo_root" rev-parse HEAD
sw_vers
system_profiler SPHardwareDataType SPDisplaysDataType
xcodebuild -version
cmake --version | head -n 1

cmake -S "$repo_root" -B "$out_dir/build" \
  -DBUILD_TESTING=ON \
  -DBEATGLASS_BUILD_DESKTOP=ON \
  -DBEATGLASS_BUILD_SLICE=ON \
  -DBEATGLASS_BUILD_MACOS_OVERLAY_TESTS=ON \
  -DBEATGLASS_ENABLE_MACOS_NATIVE_OVERLAY_TESTS=ON \
  -DBEATGLASS_BUILD_MACOS_TAP_SMOKE=ON
cmake --build "$out_dir/build" --parallel 1
desktop_plist="$out_dir/build/beatglass-desktop.app/Contents/Info.plist"
if [[ ! -f "$desktop_plist" ]]; then
  echo "Production desktop bundle plist not found: $desktop_plist" >&2
  exit 1
fi
usage_description="$(/usr/libexec/PlistBuddy -c 'Print :NSAudioCaptureUsageDescription' "$desktop_plist")"
if [[ -z "$usage_description" ]]; then
  echo "NSAudioCaptureUsageDescription is empty in the production bundle" >&2
  exit 1
fi
echo "Production bundle usage description: $usage_description"
ctest --test-dir "$out_dir/build" -R '^(macos_overlay_|macos_native_overlay$)' --output-on-failure

cat <<'EOF'

Manual cases still required; record PASS, FAIL, or UNRUN in docs/macos-qualification.md:
1. Launch the production bundle and handle the Screen & System Audio Recording prompt.
2. Play a generated tone through the selected output and run the tap smoke bundle with its UID.
3. Repeat after denial/revocation and restoration of permission.
4. Exercise menu-bar recovery, Dock reopen, display changes, click-through, fullscreen and Spaces.
EOF
