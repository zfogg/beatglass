#!/bin/bash
set -euo pipefail

# Run from the repository root on the logged-in Mac. This script does not grant
# permissions, start playback, or claim native qualification by itself.
repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
out_dir="${1:-$repo_root/build/macos-qualification}"
mkdir -p "$out_dir"
exec > >(tee "$out_dir/qualification.log") 2>&1
commands_log="$out_dir/qualification-commands.log"
: > "$commands_log"
record_command() { printf '%s\n' "$*" | tee -a "$commands_log"; }

echo "Beatglass macOS qualification evidence"
date -u +%FT%TZ
git -C "$repo_root" rev-parse HEAD
echo "Qualification output directory: $out_dir"
echo "Selected output UID: ${BEATGLASS_MACOS_TAP_UID:-UNRUN (BEATGLASS_MACOS_TAP_UID not set)}"
sw_vers
system_profiler SPHardwareDataType SPDisplaysDataType
xcodebuild -version
cmake --version | head -n 1

record_command "cmake -S $repo_root -B $out_dir/build -DBUILD_TESTING=ON -DBEATGLASS_BUILD_DESKTOP=ON -DBEATGLASS_BUILD_SLICE=ON -DBEATGLASS_BUILD_MACOS_OVERLAY_TESTS=ON -DBEATGLASS_ENABLE_MACOS_NATIVE_OVERLAY_TESTS=ON -DBEATGLASS_BUILD_MACOS_TAP_SMOKE=ON"
cmake -S "$repo_root" -B "$out_dir/build" \
  -DBUILD_TESTING=ON \
  -DBEATGLASS_BUILD_DESKTOP=ON \
  -DBEATGLASS_BUILD_SLICE=ON \
  -DBEATGLASS_BUILD_MACOS_OVERLAY_TESTS=ON \
  -DBEATGLASS_ENABLE_MACOS_NATIVE_OVERLAY_TESTS=ON \
  -DBEATGLASS_BUILD_MACOS_TAP_SMOKE=ON
record_command "cmake --build $out_dir/build --parallel 1"
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
validate_bundle_executable() {
  local bundle="$1" expected="$2" plist="$bundle/Contents/Info.plist" actual
  actual="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$plist")"
  if [[ "$actual" != "$expected" ]]; then
    echo "Bundle executable mismatch in $plist: expected $expected, got $actual" >&2
    exit 1
  fi
}
validate_bundle_executable "$out_dir/build/beatglass-desktop.app" "beatglass-desktop"
validate_bundle_executable "$tap_bundle" "beatglass-coreaudio-smoke"
validate_bundle_executable "$recovery_bundle" "beatglass-coreaudio-recovery"
echo "Native qualification artifacts: desktop=$desktop_binary tap=$tap_binary recovery=$recovery_binary"
validate_bundle_identity() {
  local bundle="$1" expected_executable="$2" expected_identifier="$3"
  local plist="$bundle/Contents/Info.plist" actual_executable actual_identifier
  actual_executable="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$plist")"
  actual_identifier="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$plist")"
  if [[ "$actual_executable" != "$expected_executable" || "$actual_identifier" != "$expected_identifier" ]]; then
    echo "Bundle identity mismatch in $plist: expected $expected_executable/$expected_identifier, got $actual_executable/$actual_identifier" >&2
    exit 1
  fi
}
validate_bundle_identity "$out_dir/build/beatglass-desktop.app" "beatglass-desktop" "org.beatglass.desktop"
validate_bundle_identity "$tap_bundle" "beatglass-coreaudio-smoke" "org.beatglass.coreaudio-smoke"
validate_bundle_identity "$recovery_bundle" "beatglass-coreaudio-recovery" "org.beatglass.coreaudio-recovery"
if [[ -n "${BEATGLASS_MACOS_TAP_UID:-}" ]]; then
  echo "Running Core Audio tap smoke for supplied UID"
  tap_log="$out_dir/tap-smoke.log"
  "$tap_binary" "$BEATGLASS_MACOS_TAP_UID" | tee "$tap_log"
  if ! grep -Eq 'frames=[1-9][0-9]* nonzero_samples=[1-9][0-9]* .*discontinuities=[0-9]+ dropped_frames=[0-9]+ timestamp_valid=1' "$tap_log"; then
    echo "Tap smoke did not report complete capture, loss, and valid timestamp counters" >&2
    exit 1
  fi
  echo "Running Core Audio recovery smoke for supplied UID"
  recovery_log="$out_dir/recovery-smoke.log"
  "$recovery_binary" "$BEATGLASS_MACOS_TAP_UID" | tee "$recovery_log"
  for cycle in 0 1; do
    if ! grep -Eq "cycle=$cycle .*frames=[1-9][0-9]* .*nonzero_samples=[1-9][0-9]* .*discontinuities=[0-9]+ .*stopped=1" "$recovery_log"; then
      echo "Recovery smoke did not complete cycle $cycle with capture and continuity evidence" >&2
      exit 1
    fi
  done
else
  echo "UNRUN: set BEATGLASS_MACOS_TAP_UID to execute tap and recovery smoke bundles"
fi
test_filter='^(macos_overlay_|macos_native_overlay$)'
record_command "ctest --test-dir $out_dir/build -N -R $test_filter"
registered_tests="$(ctest --test-dir "$out_dir/build" -N -R "$test_filter")"
printf '%s\n' "$registered_tests" > "$out_dir/overlay-ctest-discovery.log"
if ! grep -Eq 'Total Tests: [1-9][0-9]*' <<<"$registered_tests"; then
  echo "No macOS overlay tests are registered for qualification" >&2
  exit 1
fi
record_command "ctest --test-dir $out_dir/build -R $test_filter --output-on-failure"
overlay_log="$out_dir/overlay-ctest.log"
ctest --test-dir "$out_dir/build" -R "$test_filter" --output-on-failure | tee "$overlay_log"
if ! grep -Eq '[0-9]+% tests passed' "$overlay_log"; then
  echo "Overlay qualification log has no passing CTest summary" >&2
  exit 1
fi

cat <<'EOF'

Manual cases still required; record PASS, FAIL, or UNRUN in docs/macos-qualification.md:
1. Launch the production bundle and handle the Screen & System Audio Recording prompt.
2. Play a generated tone through the selected output and run the tap smoke bundle with its UID.
3. Repeat after denial/revocation and restoration of permission.
4. Exercise menu-bar recovery, Dock reopen, display changes, click-through, fullscreen and Spaces.
EOF
