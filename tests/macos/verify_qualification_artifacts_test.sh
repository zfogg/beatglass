#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "$0")/../.." && pwd)
verifier="$repo_root/tests/macos/verify_qualification_artifacts.sh"
if ! command -v shasum >/dev/null 2>&1; then
  echo "SKIP: shasum is required for macOS verifier regression"
  exit 0
fi
out_dir=$(mktemp -d "${TMPDIR:-/tmp}/beatglass-verify.XXXXXX")
trap 'rm -rf "$out_dir"' EXIT
mkdir -p "$out_dir/build/beatglass-desktop.app/Contents/MacOS" "$out_dir/build/beatglass-coreaudio-smoke.app/Contents/MacOS" "$out_dir/build/beatglass-coreaudio-recovery.app/Contents/MacOS"
printf desktop > "$out_dir/build/beatglass-desktop.app/Contents/MacOS/beatglass-desktop"
printf smoke > "$out_dir/build/beatglass-coreaudio-smoke.app/Contents/MacOS/beatglass-coreaudio-smoke"
printf recovery > "$out_dir/build/beatglass-coreaudio-recovery.app/Contents/MacOS/beatglass-coreaudio-recovery"
cat > "$out_dir/qualification-manifest.txt" <<'EOF'
captured_at=2026-01-01T00:00:00Z
commit=0123456789abcdef0123456789abcdef01234567
macos_version=test
architecture=x86_64
hardware_model=fixture
xcode_version=test
sdk_version=test
sdk_path=/sdk
developer_dir=/dev
qt_version=UNAVAILABLE
display_topology=display-topology.json
audio_inventory=audio-endpoints.json
artifact_sha256=qualification-artifacts.sha256
case_matrix=qualification-cases.tsv
EOF
printf '{"SPDisplaysDataType":[],"SPHardwareDataType":[]}' > "$out_dir/display-topology.json"
printf '{"SPAudioDataType":[]}' > "$out_dir/audio-endpoints.json"
cat > "$out_dir/qualification-cases.tsv" <<'EOF'
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
EOF
(cd "$out_dir" && shasum -a 256 build/beatglass-desktop.app/Contents/MacOS/beatglass-desktop build/beatglass-coreaudio-smoke.app/Contents/MacOS/beatglass-coreaudio-smoke build/beatglass-coreaudio-recovery.app/Contents/MacOS/beatglass-coreaudio-recovery) > "$out_dir/qualification-artifacts.sha256"
"$verifier" "$out_dir"
printf 'commit=0123456789abcdef0123456789abcdef01234567\n' >> "$out_dir/qualification-manifest.txt"
if "$verifier" "$out_dir"; then
  echo "duplicate manifest key unexpectedly passed" >&2
  exit 1
fi
sed -i '' '$d' "$out_dir/qualification-manifest.txt"
sed -i '' '2s/UNRUN/-FAIL-/' "$out_dir/qualification-cases.tsv"
if "$verifier" "$out_dir"; then
  echo "failed case without evidence unexpectedly passed" >&2
  exit 1
fi
sed -i '' '2s/-FAIL-/UNRUN/' "$out_dir/qualification-cases.tsv"
sed -i '' '2s/UNRUN/BOGUS/' "$out_dir/qualification-cases.tsv"
if "$verifier" "$out_dir"; then
  echo "invalid case status unexpectedly passed" >&2
  exit 1
fi
sed -i '' '2s/BOGUS/UNRUN/' "$out_dir/qualification-cases.tsv"
sed -i '1s/../00/' "$out_dir/qualification-artifacts.sha256"
if "$verifier" "$out_dir"; then
  echo "tampered hash unexpectedly passed" >&2
  exit 1
fi
echo "macOS qualification verifier regression checks passed"
