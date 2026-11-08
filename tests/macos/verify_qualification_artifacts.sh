#!/usr/bin/env bash
set -euo pipefail

out_dir="${1:?usage: verify_qualification_artifacts.sh <qualification-output-dir>}"
manifest="$out_dir/qualification-manifest.txt"
hashes="$out_dir/qualification-artifacts.sha256"
case_matrix="$out_dir/qualification-cases.tsv"
topology="$out_dir/display-topology.json"
audio_inventory="$out_dir/audio-endpoints.json"
[[ -s "$manifest" ]] || { echo "missing qualification manifest: $manifest" >&2; exit 1; }
[[ -s "$hashes" ]] || { echo "missing artifact hash manifest: $hashes" >&2; exit 1; }
awk -F= 'NF < 2 || $1 == "" || seen[$1]++ { bad = 1 } END { exit bad }' "$manifest" || {
  echo "qualification manifest contains malformed or duplicate keys" >&2
  exit 1
}
[[ -s "$case_matrix" ]] || { echo "missing qualification case matrix: $case_matrix" >&2; exit 1; }
[[ -s "$topology" ]] || { echo "missing or empty display topology evidence: $topology" >&2; exit 1; }
command -v plutil >/dev/null 2>&1 || { echo "plutil is required" >&2; exit 1; }
plutil -lint "$topology" >/dev/null || { echo "invalid display topology evidence: $topology" >&2; exit 1; }
[[ -s "$audio_inventory" ]] || { echo "missing or empty audio endpoint inventory: $audio_inventory" >&2; exit 1; }
plutil -lint "$audio_inventory" >/dev/null || { echo "invalid audio endpoint inventory: $audio_inventory" >&2; exit 1; }
for key in captured_at commit macos_version architecture hardware_model xcode_version sdk_version sdk_path developer_dir qt_version artifact_sha256 display_topology audio_inventory; do
  grep -q "^${key}=." "$manifest" || { echo "manifest missing ${key}" >&2; exit 1; }
done
grep -Eq '^captured_at=[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$' "$manifest" || { echo "manifest captured_at is not UTC ISO-8601" >&2; exit 1; }
commit_value=$(sed -n 's/^commit=//p' "$manifest")
[[ "$commit_value" =~ ^[0-9a-fA-F]{40}$ ]] || { echo "manifest commit is not a full SHA-1" >&2; exit 1; }
manifest_hash_ref=$(sed -n 's/^artifact_sha256=//p' "$manifest")
[[ "$manifest_hash_ref" == "qualification-artifacts.sha256" ]] || { echo "manifest artifact_sha256 must point to qualification-artifacts.sha256" >&2; exit 1; }
manifest_case_ref=$(sed -n 's/^case_matrix=//p' "$manifest")
[[ "$manifest_case_ref" == "qualification-cases.tsv" ]] || { echo "manifest case_matrix must point to qualification-cases.tsv" >&2; exit 1; }
manifest_display_ref=$(sed -n 's/^display_topology=//p' "$manifest")
manifest_audio_ref=$(sed -n 's/^audio_inventory=//p' "$manifest")
[[ "$manifest_display_ref" == "display-topology.json" && "$manifest_audio_ref" == "audio-endpoints.json" ]] || { echo "manifest machine evidence links are unexpected" >&2; exit 1; }
expected_cases=(permission_prompt capture_nonzero_timestamp permission_revoke_recovery output_switch_uid fifo_exhaustion_recovery overlay_visibility overlay_click_through_recovery overlay_display_change overlay_fullscreen_spaces menubar_dock_reopen)
mapfile -t case_lines < "$case_matrix"
[[ "${case_lines[0]:-}" == $'case_id\tstatus\tevidence' ]] || { echo "invalid qualification case matrix header" >&2; exit 1; }
[[ "${#case_lines[@]}" -eq 11 ]] || { echo "expected exactly ten qualification cases" >&2; exit 1; }
for i in "${!expected_cases[@]}"; do
  IFS=$'\t' read -r case_id status evidence <<< "${case_lines[$((i + 1))]}"
  [[ "$case_id" == "${expected_cases[$i]}" ]] || { echo "unexpected qualification case: $case_id" >&2; exit 1; }
  [[ "$status" =~ ^(PASS|FAIL|UNRUN)$ ]] || { echo "invalid status for $case_id: $status" >&2; exit 1; }
  if [[ "$status" == PASS ]]; then
    [[ -n "$evidence" && "$evidence" != /* ]] || { echo "PASS case lacks relative evidence: $case_id" >&2; exit 1; }
    evidence_path=$(cd "$out_dir" && cd "$(dirname -- "$evidence")" 2>/dev/null && pwd)/$(basename -- "$evidence") || { echo "invalid evidence path: $evidence" >&2; exit 1; }
    case "$evidence_path" in
      "$out_dir"/*) ;;
      *) echo "evidence path escapes output: $evidence" >&2; exit 1;;
    esac
    [[ -s "$evidence_path" ]] || { echo "missing or empty evidence file for $case_id: $evidence" >&2; exit 1; }
    [[ ! -L "$evidence_path" ]] || { echo "evidence must not be a symlink: $evidence" >&2; exit 1; }
  fi
done
command -v shasum >/dev/null 2>&1 || { echo "shasum is required" >&2; exit 1; }
mapfile -t hash_lines < "$hashes"
[[ "${#hash_lines[@]}" -eq 3 ]] || { echo "expected exactly three artifact hashes" >&2; exit 1; }
declare -A seen=()
expected_artifacts=("build/beatglass-desktop.app/Contents/MacOS/beatglass-desktop" "build/beatglass-coreaudio-smoke.app/Contents/MacOS/beatglass-coreaudio-smoke" "build/beatglass-coreaudio-recovery.app/Contents/MacOS/beatglass-coreaudio-recovery")
declare -A expected_seen=()
for line in "${hash_lines[@]}"; do
  [[ "$line" =~ ^([[:xdigit:]]{64})[[:space:]][[:space:]](.+)$ ]] || {
    echo "malformed artifact hash line: $line" >&2; exit 1;
  }
  expected="${BASH_REMATCH[1]}"
  relative="${BASH_REMATCH[2]}"
  normalized_relative="${relative//\\//}"
  for expected in "${expected_artifacts[@]}"; do [[ "$normalized_relative" == "$expected" ]] && expected_seen[$expected]=1; done
  [[ "$relative" != /* && "$relative" != *$'\n'* ]] || {
    echo "artifact path must be relative: $relative" >&2; exit 1;
  }
  candidate=$(cd "$out_dir" && cd "$(dirname -- "$relative")" 2>/dev/null && pwd)/$(basename -- "$relative") || {
    echo "invalid artifact path: $relative" >&2; exit 1;
  }
  root=$(cd "$out_dir" && pwd)
  case "$candidate" in
    "$root"/*) ;;
    *) echo "artifact path escapes output: $relative" >&2; exit 1;;
  esac
  [[ -z "${seen[$candidate]:-}" ]] || { echo "duplicate artifact path: $relative" >&2; exit 1; }
  seen[$candidate]=1
  [[ -f "$candidate" ]] || { echo "missing artifact: $candidate" >&2; exit 1; }
  [[ ! -L "$candidate" ]] || { echo "artifact must not be a symlink: $relative" >&2; exit 1; }
  actual=$(shasum -a 256 "$candidate" | awk '{print $1}')
  [[ "$actual" == "$expected" ]] || {
    echo "hash mismatch for $relative (expected $expected, got $actual)" >&2; exit 1;
  }
done
for expected in "${expected_artifacts[@]}"; do [[ -n "${expected_seen[$expected]:-}" ]] || { echo "missing expected artifact: $expected" >&2; exit 1; }; done
echo "qualification artifact hashes verified: $out_dir"
