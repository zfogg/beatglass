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
p=''; m=''; player=''; player2=''; test_pid=''
cleanup() {
 for pid in "$test_pid" "$player2" "$player" "$m" "$p"; do
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
python3 - "$XDG_RUNTIME_DIR/tone.wav" "$XDG_RUNTIME_DIR/tone2.wav" <<'PY'
import math, struct, sys, wave
for path, hz in zip(sys.argv[1:], (440, 880)):
    with wave.open(path, 'wb') as out:
        out.setparams((2, 2, 48000, 0, 'NONE', 'not compressed'))
        out.writeframes(b''.join(struct.pack('<hh', int(12000*math.sin(2*math.pi*hz*i/48000)), int(12000*math.sin(2*math.pi*hz*i/48000))) for i in range(48000*8)))
PY
other_id=$(pw-dump | python3 -c 'import sys,json; print(next(x["id"] for x in json.load(sys.stdin) if x.get("info",{}).get("props",{}).get("node.name")=="beatglass_other_sink"))')
wpctl set-default "$other_id"
read -r sink_id sink_serial < <(pw-dump | python3 -c 'import sys,json; x=next(x for x in json.load(sys.stdin) if x.get("type")=="PipeWire:Interface:Node" and x.get("info",{}).get("props",{}).get("node.name")=="beatglass_test_sink"); print(x["id"], x["info"]["props"]["object.serial"])')
pw-play --target "$sink_id" "$XDG_RUNTIME_DIR/tone.wav" >"$XDG_RUNTIME_DIR/play.log" 2>&1 & player=$!
"$1" "$XDG_RUNTIME_DIR/capturing" "${2:-remove}" & test_pid=$!
ready=0
for _ in $(seq 1 200); do
 if [ -f "$XDG_RUNTIME_DIR/capturing" ]; then ready=1; break; fi
 if ! kill -0 "$test_pid" 2>/dev/null; then break; fi
 sleep 0.05
done
if [ "$ready" != 1 ]; then wait "$test_pid" || true; cat "$XDG_RUNTIME_DIR/session.log"; exit 1; fi
if [ "${2:-remove}" = inject ]; then
 pw-play --target "$other_id" "$XDG_RUNTIME_DIR/tone2.wav" >"$XDG_RUNTIME_DIR/injected-play.log" 2>&1 & player2=$!
 ready=0
 for _ in $(seq 1 100); do
  if pw-dump | python3 -c 'import json,sys; n=int(sys.argv[1]); sys.exit(0 if any(x.get("type")=="PipeWire:Interface:Link" and x.get("info",{}).get("input-node-id")==n and x.get("info",{}).get("state")=="active" for x in json.load(sys.stdin)) else 1)' "$other_id"; then ready=1; break; fi
  sleep 0.05
 done
 if [ "$ready" != 1 ]; then cat "$XDG_RUNTIME_DIR/injected-play.log"; exit 1; fi
 read -r source_port input_port < <(pw-dump | python3 -c 'import json,sys; d=json.load(sys.stdin); other=int(sys.argv[1]); stream=next(x["id"] for x in d if x.get("type")=="PipeWire:Interface:Node" and x.get("info",{}).get("props",{}).get("media.name")=="Beatglass output monitor"); ports=[x for x in d if x.get("type")=="PipeWire:Interface:Port"]; find=lambda node,channel,monitor: next(x["id"] for x in ports if x.get("info",{}).get("props",{}).get("node.id")==node and x.get("info",{}).get("props",{}).get("audio.channel")==channel and bool(x.get("info",{}).get("props",{}).get("port.monitor",False))==monitor); print(find(other,"FL",True),find(stream,"FL",False))' "$other_id")
 pw-link "$source_port" "$input_port"
else
 pw-cli destroy "$sink_id"
 ready=0
 for _ in $(seq 1 100); do
  if [ -f "$XDG_RUNTIME_DIR/capturing.lost" ]; then ready=1; break; fi
  if ! kill -0 "$test_pid" 2>/dev/null; then break; fi
  sleep 0.05
 done
 if [ "$ready" != 1 ]; then wait "$test_pid" || true; echo "loss event not observed"; exit 1; fi
 pw-cli create-node adapter '{ factory.name = support.null-audio-sink node.name = beatglass_test_sink media.class = Audio/Sink audio.position = [ FL FR ] node.driver = true object.linger = true }'
 ready=0
 for _ in $(seq 1 100); do
  if pw-dump | python3 -c 'import json,sys; old=str(sys.argv[1]); sys.exit(0 if any(x.get("type")=="PipeWire:Interface:Node" and x.get("info",{}).get("props",{}).get("node.name")=="beatglass_test_sink" and str(x.get("info",{}).get("props",{}).get("object.serial"))!=old for x in json.load(sys.stdin)) else 1)' "$sink_serial"; then ready=1; break; fi
  sleep 0.05
 done
 if [ "$ready" != 1 ]; then echo "replacement sink unavailable"; exit 1; fi
 touch "$XDG_RUNTIME_DIR/capturing.replaced"
fi
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
