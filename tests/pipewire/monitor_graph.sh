#!/usr/bin/env bash
set -eu
if [ "${BG_TEST_SESSION:-}" != ready ]; then
 export XDG_RUNTIME_DIR="$(mktemp -d)"
 export BG_TEST_SESSION=ready
 exec dbus-run-session -- bash "$0" "$@"
fi
pipewire >"$XDG_RUNTIME_DIR/pw.log" 2>&1 & p=$!
wireplumber >"$XDG_RUNTIME_DIR/session.log" 2>&1 & m=$!
v=''; player=''
cleanup() {
 for pid in "$player" "$v" "$m" "$p"; do
  if [ -n "$pid" ]; then kill "$pid" 2>/dev/null || true; fi
 done
 wait 2>/dev/null || true
}
trap cleanup EXIT
sleep 2
pw-loopback --capture-props='{ media.class = Audio/Sink node.name = beatglass_test_sink }' --playback-props='{ node.name = beatglass_test_loop }' >"$XDG_RUNTIME_DIR/loop.log" 2>&1 & v=$!
sleep 2
python3 - "$XDG_RUNTIME_DIR/tone.wav" <<'PY'
import math, struct, sys, wave
with wave.open(sys.argv[1], 'wb') as out:
    out.setparams((2, 2, 48000, 0, 'NONE', 'not compressed'))
    out.writeframes(b''.join(struct.pack('<hh', int(18000*math.sin(2*math.pi*440*i/48000)), int(18000*math.sin(2*math.pi*440*i/48000))) for i in range(48000*8)))
PY
sink_id=$(pw-dump | python3 -c 'import sys,json; print(next(x["id"] for x in json.load(sys.stdin) if x.get("info",{}).get("props",{}).get("node.name")=="beatglass_test_sink"))')
pw-play --target "$sink_id" "$XDG_RUNTIME_DIR/tone.wav" >"$XDG_RUNTIME_DIR/play.log" 2>&1 & player=$!
"$1" & test_pid=$!
sleep 3
wait "$test_pid"
