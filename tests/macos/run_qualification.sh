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
record_command "sw_vers"
sw_vers
record_command "system_profiler SPHardwareDataType SPDisplaysDataType"
system_profiler SPHardwareDataType SPDisplaysDataType
topology_path="$out_dir/display-topology.json"
record_command "system_profiler SPHardwareDataType SPDisplaysDataType -json > $topology_path"
system_profiler SPHardwareDataType SPDisplaysDataType -json > "$topology_path"
[[ -s "$topology_path" ]] || { echo "display topology evidence is empty" >&2; exit 1; }
echo "Display topology evidence: $topology_path"
audio_inventory_path="$out_dir/audio-endpoints.json"
record_command "system_profiler SPAudioDataType -json > $audio_inventory_path"
system_profiler SPAudioDataType -json > "$audio_inventory_path"
[[ -s "$audio_inventory_path" ]] || { echo "audio endpoint inventory is empty" >&2; exit 1; }
echo "Audio endpoint inventory: $audio_inventory_path"
record_command "xcodebuild -version"
xcodebuild -version
record_command "cmake --version"
cmake --version | head -n 1
sdk_path="$(xcrun --sdk macosx --show-sdk-path)"
if [[ ! -d "$sdk_path" ]]; then
  echo "macOS SDK path is missing: $sdk_path" >&2
  exit 1
fi
developer_dir="$(xcode-select -p)"
if [[ ! -d "$developer_dir" ]]; then
  echo "Active Xcode developer directory is missing: $developer_dir" >&2
  exit 1
fi
record_command "xcrun --sdk macosx --show-sdk-path"
record_command "xcrun --sdk macosx --show-sdk-version"
record_command "xcode-select -p"

# CMake's slice configuration needs aubio headers. Fail with the native
# package-manager remediation instead of leaving a less actionable find_path
# error several lines later.
aubio_header=""
for candidate in \
  "${AUBIO_INCLUDE_DIR:-}/aubio/aubio.h" \
  "$(brew --prefix aubio 2>/dev/null || true)/include/aubio/aubio.h" \
  /opt/homebrew/include/aubio/aubio.h \
  /usr/local/include/aubio/aubio.h; do
  if [[ -n "$candidate" && -f "$candidate" ]]; then
    aubio_header="$candidate"
    break
  fi
done
if [[ -z "$aubio_header" ]]; then
  echo "aubio/aubio.h is required for the macOS qualification build but was not found." >&2
  echo "Install it with: brew install aubio" >&2
  echo "Then rerun: tests/macos/run_qualification.sh $out_dir" >&2
  exit 1
fi
echo "aubio header: $aubio_header"

record_command "cmake -S $repo_root -B $out_dir/build -DBUILD_TESTING=ON -DBEATGLASS_BUILD_TUI=ON -DBEATGLASS_BUILD_DESKTOP=ON -DBEATGLASS_BUILD_DESKTOP_ANALYSIS=ON -DBEATGLASS_BUILD_SLICE=ON -DBEATGLASS_BUILD_LIBRARY_PROTOTYPE=ON -DBEATGLASS_BUILD_MACOS_OVERLAY_TESTS=ON -DBEATGLASS_ENABLE_MACOS_NATIVE_OVERLAY_TESTS=ON -DBEATGLASS_BUILD_MACOS_TAP_SMOKE=ON"
cmake -S "$repo_root" -B "$out_dir/build" \
  -DBUILD_TESTING=ON \
  -DBEATGLASS_BUILD_TUI=ON \
  -DBEATGLASS_BUILD_DESKTOP=ON \
  -DBEATGLASS_BUILD_DESKTOP_ANALYSIS=ON \
  -DBEATGLASS_BUILD_SLICE=ON \
  -DBEATGLASS_BUILD_LIBRARY_PROTOTYPE=ON \
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
usage_description_lower="$(printf '%s' "$usage_description" | tr '[:upper:]' '[:lower:]')"
if [[ -z "$usage_description" || "$usage_description_lower" != *system* || "$usage_description_lower" != *output* || "$usage_description_lower" == *microphone* || "$usage_description_lower" == *camera* ]]; then
  echo "NSAudioCaptureUsageDescription is empty in the production bundle" >&2
  exit 1
fi
echo "Production bundle usage description: $usage_description"
desktop_binary="$out_dir/build/beatglass-desktop.app/Contents/MacOS/beatglass-desktop"
cli_binary="$out_dir/build/beatglass-cli"
tool_binary="$out_dir/build/beatglass-tool"
tap_bundle="$out_dir/build/beatglass-coreaudio-smoke.app"
tap_binary="$tap_bundle/Contents/MacOS/beatglass-coreaudio-smoke"
recovery_bundle="$out_dir/build/beatglass-coreaudio-recovery.app"
recovery_binary="$recovery_bundle/Contents/MacOS/beatglass-coreaudio-recovery"
for required_path in "$desktop_binary" "$cli_binary" "$tool_binary" "$tap_bundle" "$tap_binary" "$recovery_bundle" "$recovery_binary"; do
  if [[ "$required_path" == *.app && ! -d "$required_path" ]] ||
     [[ "$required_path" != *.app && ! -x "$required_path" ]]; then
    echo "Required native qualification artifact not found: $required_path" >&2
    exit 1
  fi
done
for native_binary in "$desktop_binary" "$cli_binary" "$tool_binary" "$tap_binary" "$recovery_binary"; do
  if ! file -b "$native_binary" | grep -q 'Mach-O'; then
    echo "Qualification artifact is not a native Mach-O executable: $native_binary" >&2
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
  bundle_usage_lower="$(printf '%s' "$bundle_usage" | tr '[:upper:]' '[:lower:]')"
  if [[ -z "$bundle_usage" || "$bundle_usage_lower" != *system* || "$bundle_usage_lower" != *output* || "$bundle_usage_lower" == *microphone* || "$bundle_usage_lower" == *camera* ]]; then
    echo "NSAudioCaptureUsageDescription is empty in $bundle_plist" >&2
    exit 1
  fi
done
validate_bundle_executable() {
  local bundle expected plist actual
  bundle="$1"
  expected="$2"
  plist="$bundle/Contents/Info.plist"
  actual="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$plist")"
  if [[ "$actual" != "$expected" ]]; then
    echo "Bundle executable mismatch in $plist: expected $expected, got $actual" >&2
    exit 1
  fi
}
validate_bundle_executable "$out_dir/build/beatglass-desktop.app" "beatglass-desktop"
validate_bundle_executable "$tap_bundle" "beatglass-coreaudio-smoke"
validate_bundle_executable "$recovery_bundle" "beatglass-coreaudio-recovery"
echo "Native qualification artifacts: desktop=$desktop_binary cli=$cli_binary tool=$tool_binary tap=$tap_binary recovery=$recovery_binary"
validate_bundle_identity() {
  local bundle="$1" expected_executable="$2" expected_identifier="$3" expected_name="$4"
  local plist="$bundle/Contents/Info.plist" actual_executable actual_identifier actual_name package_type bundle_version
  actual_executable="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$plist")"
  actual_identifier="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$plist")"
  actual_name="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleName' "$plist")"
  package_type="$(/usr/libexec/PlistBuddy -c 'Print :CFBundlePackageType' "$plist")"
  bundle_version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "$plist")"
  if [[ "$actual_executable" != "$expected_executable" || "$actual_identifier" != "$expected_identifier" || "$actual_name" != "$expected_name" || "$package_type" != "APPL" || ! "$bundle_version" =~ ^[0-9]+([.][0-9]+)*$ ]]; then
    echo "Bundle identity mismatch in $plist: expected $expected_executable/$expected_identifier, got $actual_executable/$actual_identifier" >&2
    exit 1
  fi
}
validate_bundle_identity "$out_dir/build/beatglass-desktop.app" "beatglass-desktop" "org.beatglass.desktop" "Beatglass"
validate_bundle_identity "$tap_bundle" "beatglass-coreaudio-smoke" "org.beatglass.coreaudio-smoke" "Beatglass Core Audio Smoke"
validate_bundle_identity "$recovery_bundle" "beatglass-coreaudio-recovery" "org.beatglass.coreaudio-recovery" "Beatglass Core Audio Recovery"
for bundle in "$out_dir/build/beatglass-desktop.app" "$tap_bundle" "$recovery_bundle"; do
  bundle_plist="$bundle/Contents/Info.plist"
  for forbidden_key in NSMicrophoneUsageDescription NSCameraUsageDescription; do
    if forbidden_usage="$(/usr/libexec/PlistBuddy -c "Print :$forbidden_key" "$bundle_plist" 2>/dev/null)" && [[ -n "$forbidden_usage" ]]; then
      echo "$forbidden_key is not allowed in qualification bundle: $bundle_plist" >&2
      exit 1
    fi
  done
done
output_discovery_log="$out_dir/output-discovery.log"
record_command "$tap_binary"
"$tap_binary" | tee "$output_discovery_log"
if [[ ! -s "$output_discovery_log" ]]; then
  echo "Core Audio output discovery log is missing or empty" >&2
  exit 1
fi
if ! awk 'NF >= 2 { if (++seen[$1] > 1) exit 1 }' "$output_discovery_log"; then
  echo "Core Audio output discovery contains duplicate UIDs" >&2
  exit 1
fi
if [[ -n "${BEATGLASS_MACOS_TAP_UID:-}" ]] && ! awk -v uid="$BEATGLASS_MACOS_TAP_UID" '$1 == uid { found = 1 } END { exit !found }' "$output_discovery_log"; then
  echo "Selected output UID was not present in current Core Audio discovery" >&2
  exit 1
fi
if [[ -n "${BEATGLASS_MACOS_TAP_UID:-}" ]]; then
  echo "Running Core Audio tap smoke for supplied UID"
  record_command "$tap_binary $BEATGLASS_MACOS_TAP_UID"
  tap_log="$out_dir/tap-smoke.log"
  "$tap_binary" "$BEATGLASS_MACOS_TAP_UID" | tee "$tap_log"
  if ! grep -Eq 'frames=[1-9][0-9]* nonzero_samples=[1-9][0-9]* .*discontinuities=[0-9]+ dropped_frames=[0-9]+ timestamp_valid=1' "$tap_log"; then
    echo "Tap smoke did not report complete capture, loss, and valid timestamp counters" >&2
    exit 1
  fi
  echo "Running Core Audio recovery smoke for supplied UID"
  record_command "$recovery_binary $BEATGLASS_MACOS_TAP_UID"
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
cli_test_filter='^(issue4320_tui_contract|cli_generated_audio|cli_v1_codecs|cli_tui_snapshots)$'
record_command "ctest --test-dir $out_dir/build -R $cli_test_filter --output-on-failure"
cli_log="$out_dir/cli-ctest.log"
ctest --test-dir "$out_dir/build" -R "$cli_test_filter" --output-on-failure | tee "$cli_log"
if ! grep -Eq '[0-9]+% tests passed' "$cli_log"; then
  echo "CLI qualification log has no passing CTest summary" >&2
  exit 1
fi
for evidence in "$commands_log" "$output_discovery_log" "$out_dir/overlay-ctest-discovery.log" "$overlay_log" "$cli_log"; do
  if [[ ! -s "$evidence" ]]; then
    echo "Qualification evidence file is missing or empty: $evidence" >&2
    exit 1
  fi
done
if [[ -n "${BEATGLASS_MACOS_TAP_UID:-}" ]]; then
  for evidence in "$out_dir/tap-smoke.log" "$out_dir/recovery-smoke.log"; do
    if [[ ! -s "$evidence" ]]; then
      echo "Capture evidence file is missing or empty: $evidence" >&2
      exit 1
    fi
  done
fi

manifest="$out_dir/qualification-manifest.txt"
artifact_manifest="$out_dir/qualification-artifacts.sha256"
qt_version="UNAVAILABLE"
if command -v qmake >/dev/null 2>&1; then qt_version="$(qmake --version | tail -n 1)"; fi
permission_result="${BEATGLASS_MACOS_PERMISSION_RESULT:-UNRUN}"
if [[ "$permission_result" != "allowed" && "$permission_result" != "denied" && "$permission_result" != "UNRUN" ]]; then
  echo "BEATGLASS_MACOS_PERMISSION_RESULT must be allowed, denied, or UNRUN" >&2
  exit 1
fi
if [[ "$permission_result" == "allowed" && -z "${BEATGLASS_MACOS_TAP_UID:-}" ]]; then
  echo "An allowed permission result requires BEATGLASS_MACOS_TAP_UID" >&2
  exit 1
fi
signing_log="$out_dir/codesign-desktop.log"
signing_state=unverified
record_command "codesign --display --verbose=4 $out_dir/build/beatglass-desktop.app"
codesign --display --verbose=4 "$out_dir/build/beatglass-desktop.app" >"$signing_log" 2>&1 || true
record_command "codesign --verify --deep --strict $out_dir/build/beatglass-desktop.app"
if codesign --verify --deep --strict "$out_dir/build/beatglass-desktop.app" 2>>"$signing_log"; then
  signing_state=verified
fi
tap_signing_log="$out_dir/codesign-tap.log"
recovery_signing_log="$out_dir/codesign-recovery.log"
record_command "codesign --display --verbose=4 $tap_bundle"
codesign --display --verbose=4 "$tap_bundle" >"$tap_signing_log" 2>&1 || true
record_command "codesign --display --verbose=4 $recovery_bundle"
codesign --display --verbose=4 "$recovery_bundle" >"$recovery_signing_log" 2>&1 || true
tap_signing=unverified
recovery_signing=unverified
record_command "codesign --verify --deep --strict $tap_bundle"
if codesign --verify --deep --strict "$tap_bundle" 2>>"$tap_signing_log"; then tap_signing=verified; fi
record_command "codesign --verify --deep --strict $recovery_bundle"
if codesign --verify --deep --strict "$recovery_bundle" 2>>"$recovery_signing_log"; then recovery_signing=verified; fi
if [[ "${BEATGLASS_MACOS_REQUIRE_SIGNING:-0}" == "1" ]]; then
  for signed_state in "$signing_state" "$tap_signing" "$recovery_signing"; do
    if [[ "$signed_state" != "verified" ]]; then
      echo "BEATGLASS_MACOS_REQUIRE_SIGNING=1 requires all qualification bundles to pass codesign verification" >&2
      exit 1
    fi
  done
fi
{
  printf 'captured_at=%s\n' "$(date -u +%FT%TZ)"
  printf 'commit=%s\n' "$(git -C "$repo_root" rev-parse HEAD)"
  printf 'macos_version=%s\n' "$(sw_vers -productVersion)"
  printf 'architecture=%s\n' "$(uname -m)"
  printf 'hardware_model=%s\n' "$(system_profiler SPHardwareDataType | awk -F': ' '/Model Name|Model Identifier/ {print $2; exit}')"
  printf 'xcode_version=%s\n' "$(xcodebuild -version | tr '\n' ' ')"
  printf 'sdk_version=%s\n' "$(xcrun --sdk macosx --show-sdk-version)"
  printf 'sdk_path=%s\n' "$sdk_path"
  printf 'developer_dir=%s\n' "$developer_dir"
  printf 'qt_version=%s\n' "$qt_version"
  printf 'display_topology=display-topology.json\naudio_inventory=audio-endpoints.json\n'
  printf 'desktop_signing=%s\ntap_signing=%s\nrecovery_signing=%s\ndesktop_signing_log=%s\ntap_signing_log=%s\nrecovery_signing_log=%s\n' "$signing_state" "$tap_signing" "$recovery_signing" "$signing_log" "$tap_signing_log" "$recovery_signing_log"
  printf 'selected_output_uid=%s\n' "${BEATGLASS_MACOS_TAP_UID:-UNRUN}"
  printf 'permission_result=%s\n' "$permission_result"
  printf 'commands=%s\noutput_discovery=%s\noverlay_discovery=%s\noverlay_output=%s\ncli_output=%s\n' "$commands_log" "$output_discovery_log" "$out_dir/overlay-ctest-discovery.log" "$overlay_log" "$cli_log"
  if [[ -n "${BEATGLASS_MACOS_TAP_UID:-}" ]]; then
    printf 'tap_output=%s\nrecovery_output=%s\n' "$out_dir/tap-smoke.log" "$out_dir/recovery-smoke.log"
  fi
} > "$manifest"
: > "$artifact_manifest"
for artifact in "$desktop_binary" "$tap_binary" "$recovery_binary"; do
  artifact_rel=${artifact#"$out_dir"/}
  [[ "$artifact_rel" != "$artifact" && "$artifact_rel" != /* ]] || {
    echo "Qualification artifact is outside output directory: $artifact" >&2
    exit 1
  }
  (cd "$out_dir" && shasum -a 256 "$artifact_rel") >> "$artifact_manifest"
done
if [[ ! -s "$artifact_manifest" ]] || [[ "$(wc -l < "$artifact_manifest" | tr -d ' ')" -ne 3 ]]; then
  echo "Qualification artifact hash manifest is missing or incomplete" >&2
  exit 1
fi
printf 'artifact_sha256=%s\n' "$artifact_manifest" >> "$manifest"
case_matrix="$out_dir/qualification-cases.tsv"
cat > "$case_matrix" <<'CASES'
case_id	status	evidence
permission_prompt	UNRUN	-
capture_nonzero_timestamp	UNRUN	-
permission_revoke_recovery	UNRUN	-
output_switch_uid	UNRUN	-
fifo_exhaustion_recovery	UNRUN	-
overlay_visibility	UNRUN	-
overlay_click_through_recovery	UNRUN	-
overlay_display_change	UNRUN	-
overlay_fullscreen_spaces	UNRUN	-
menubar_dock_reopen	UNRUN	-
CASES
printf 'case_matrix=%s\n' "$case_matrix" >> "$manifest"
if [[ ! -s "$manifest" ]] || ! grep -q '^commit=.' "$manifest" || ! grep -q '^macos_version=.' "$manifest" || ! grep -q '^architecture=.' "$manifest" || ! grep -q '^xcode_version=.' "$manifest" || ! grep -q '^sdk_version=.' "$manifest" || ! grep -q '^sdk_path=.' "$manifest" || ! grep -q '^developer_dir=.' "$manifest" || ! grep -q '^desktop_signing=\(verified\|unverified\)$' "$manifest" || ! grep -q '^tap_signing=\(verified\|unverified\)$' "$manifest" || ! grep -q '^recovery_signing=\(verified\|unverified\)$' "$manifest" || [[ ! -s "$signing_log" || ! -s "$tap_signing_log" || ! -s "$recovery_signing_log" ]]; then
  echo "Qualification manifest is missing or empty" >&2
  exit 1
fi

cat <<'EOF'

Manual cases still required; record PASS, FAIL, or UNRUN in docs/macos-qualification.md:
1. Launch the production bundle and handle the Screen & System Audio Recording prompt.
2. Play a generated tone through the selected output and run the tap smoke bundle with its UID.
3. Repeat after denial/revocation and restoration of permission.
4. Exercise menu-bar recovery, Dock reopen, display changes, click-through, fullscreen and Spaces.
EOF
