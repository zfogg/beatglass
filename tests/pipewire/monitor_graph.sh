#!/usr/bin/env bash
set -eu
if [ "${BG_TEST_SESSION:-}" != ready ]; then
 export XDG_RUNTIME_DIR="$(mktemp -d)"
 export BG_TEST_SESSION=ready
 exec dbus-run-session -- bash "$0" "$@"
fi
export XDG_CONFIG_HOME="$XDG_RUNTIME_DIR/config"
mkdir -p "$XDG_CONFIG_HOME/wireplumber/bluetooth.lua.d"
printf '%s\n' '-- Bluetooth is unnecessary in this virtual graph test' > "$XDG_CONFIG_HOME/wireplumber/bluetooth.lua.d/90-enable-all.lua"
mkdir -p "$XDG_CONFIG_HOME/pipewire/pipewire.conf.d"
cat > "$XDG_CONFIG_HOME/pipewire/pipewire.conf.d/beatglass-test.conf" <<'CONF'
context.objects = [
  { factory = adapter
    args = { factory.name = support.null-audio-sink
             node.name = beatglass_test_sink
             node.description = "Beatglass test sink"
             media.class = Audio/Sink
             audio.position = [ FL FR ]
             node.driver = true }
  }
  { factory = adapter
    args = { factory.name = support.null-audio-sink
             node.name = beatglass_other_sink
             node.description = "Beatglass other sink"
             media.class = Audio/Sink
             audio.position = [ FL FR ]
             node.driver = true }
  }
]
CONF
p=''; m=''; player=''; test_pid=''
cleanup() {
 for pid in "$test_pid" "$player" "$m" "$p"; do
  if [ -n "$pid" ]; then kill "$pid" 2>/dev/null || true; fi
 done
 wait 2>/dev/null || true
}
trap cleanup EXIT
pipewire >"$XDG_RUNTIME_DIR/pw.log" 2>&1 & p=$!
ready=0
for _ in $(seq 1 100); do
 if pw-cli info 0 >/dev/null 2>&1; then ready=1; break; fi
 sleep 0.05
done
if [ "$ready" != 1 ]; then cat "$XDG_RUNTIME_DIR/pw.log"; exit 1; fi
wireplumber >"$XDG_RUNTIME_DIR/session.log" 2>&1 & m=$!
ready=0
for _ in $(seq 1 100); do
 if wpctl status >/dev/null 2>&1; then ready=1; break; fi
 sleep 0.05
done
if [ "$ready" != 1 ]; then cat "$XDG_RUNTIME_DIR/session.log"; exit 1; fi
python3 - "$XDG_RUNTIME_DIR/tone.wav" <<'PY'
import math, struct, sys, wave
with wave.open(sys.argv[1], 'wb') as out:
    out.setparams((2, 2, 48000, 0, 'NONE', 'not compressed'))
    out.writeframes(b''.join(struct.pack('<hh', int(18000*math.sin(2*math.pi*440*i/48000)), int(18000*math.sin(2*math.pi*440*i/48000))) for i in range(48000*8)))
PY
other_id=$(pw-dump | python3 -c 'import sys,json; print(next(x["id"] for x in json.load(sys.stdin) if x.get("info",{}).get("props",{}).get("node.name")=="beatglass_other_sink"))')
wpctl set-default "$other_id"
sink_id=$(pw-dump | python3 -c 'import sys,json; print(next(x["id"] for x in json.load(sys.stdin) if x.get("info",{}).get("props",{}).get("node.name")=="beatglass_test_sink"))')
pw-play --target "$sink_id" "$XDG_RUNTIME_DIR/tone.wav" >"$XDG_RUNTIME_DIR/play.log" 2>&1 & player=$!
"$1" & test_pid=$!
if ! wait "$test_pid"; then
 cat "$XDG_RUNTIME_DIR/session.log"
 exit 1
fi
test_pid=''
kill "$player" 2>/dev/null || true
wait "$player" 2>/dev/null || true
player=''
pw-play --target "$other_id" "$XDG_RUNTIME_DIR/tone.wav" >"$XDG_RUNTIME_DIR/play2.log" 2>&1 & player=$!
ready=0
for _ in $(seq 1 100); do
 if pw-dump | python3 -c 'import json,sys; n=int(sys.argv[1]); sys.exit(0 if any(x.get("type")=="PipeWire:Interface:Link" and x.get("info",{}).get("input-node-id")==n and x.get("info",{}).get("state")=="active" for x in json.load(sys.stdin)) else 1)' "$other_id"; then ready=1; break; fi
 sleep 0.05
done
if [ "$ready" != 1 ]; then cat "$XDG_RUNTIME_DIR/play2.log"; exit 1; fi
"$1" missing
