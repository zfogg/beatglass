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
echo "Qualification output directory: $out_dir"
echo "Selected output UID: ${BEATGLASS_MACOS_TAP_UID:-UNRUN (BEATGLASS_MACOS_TAP_UID not set)}"
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
desktop_binary="$out_dir/build/beatglass-desktop.app/Contents/MacOS/beatglass-desktop"
tap_bundle="$out_dir/build/beatglass-coreaudio-smoke.app"
tap_binary="$tap_bundle/Contents/MacOS/beatglass-coreaudio-smoke"
recovery_bundle="$out_dir/build/beatglass-coreaudio-recovery.app"
recovery_binary="$recovery_bundle/Contents/MacOS/beatglass-coreaudio-recovery"
for required_path in "$desktop_binary" "$tap_bundle" "$tap_binary" "$recovery_bundle" "$recovery_binary"; do
  if [[ ! -e "$required_path" ]]; then
    echo "Required native qualification artifact not found: $required_path" >&2
    exit 1
  fi
done
for bundle in "$tap_bundle" "$recovery_bundle"; do
  bundle_plist="$bundle/Contents/Info.plist"
  if [[ ! -f "$bundle_plist" ]]; then
    echo "Native smoke bundle plist not found: $bundle_plist" >&2
    exit 1
  fi
  bundle_usage="$(/usr/libexec/PlistBuddy -c 'Print :NSAudioCaptureUsageDescription' "$bundle_plist")"
  if [[ -z "$bundle_usage" ]]; then
    echo "NSAudioCaptureUsageDescription is empty in $bundle_plist" >&2
    exit 1
  fi
done
echo "Native qualification artifacts: desktop=$desktop_binary tap=$tap_binary recovery=$recovery_binary"
if [[ -n "${BEATGLASS_MACOS_TAP_UID:-}" ]]; then
  echo "Running Core Audio tap smoke for supplied UID"
  tap_log="$out_dir/tap-smoke.log"
  "$tap_binary" "$BEATGLASS_MACOS_TAP_UID" | tee "$tap_log"
  if ! grep -Eq 'frames=[1-9][0-9]* nonzero_samples=[1-9][0-9]* .*discontinuities=[0-9]+ dropped_frames=[0-9]+' "$tap_log"; then
    echo "Tap smoke did not report complete nonzero capture and loss counters" >&2
    exit 1
  fi
  echo "Running Core Audio recovery smoke for supplied UID"
  recovery_log="$out_dir/recovery-smoke.log"
  "$recovery_binary" "$BEATGLASS_MACOS_TAP_UID" | tee "$recovery_log"
  for cycle in 0 1; do
    if ! grep -Eq "cycle=$cycle .*nonzero_samples=[1-9][0-9]* .*stopped=1" "$recovery_log"; then
      echo "Recovery smoke did not complete cycle $cycle with nonzero samples" >&2
      exit 1
    fi
  done
else
  echo "UNRUN: set BEATGLASS_MACOS_TAP_UID to execute tap and recovery smoke bundles"
fi
test_filter='^(macos_overlay_|macos_native_overlay$)'
registered_tests="$(ctest --test-dir "$out_dir/build" -N -R "$test_filter")"
if ! grep -Eq 'Total Tests: [1-9][0-9]*' <<<"$registered_tests"; then
  echo "No macOS overlay tests are registered for qualification" >&2
  exit 1
fi
ctest --test-dir "$out_dir/build" -R "$test_filter" --output-on-failure

cat <<'EOF'

Manual cases still required; record PASS, FAIL, or UNRUN in docs/macos-qualification.md:
1. Launch the production bundle and handle the Screen & System Audio Recording prompt.
2. Play a generated tone through the selected output and run the tap smoke bundle with its UID.
3. Repeat after denial/revocation and restoration of permission.
4. Exercise menu-bar recovery, Dock reopen, display changes, click-through, fullscreen and Spaces.
EOF
