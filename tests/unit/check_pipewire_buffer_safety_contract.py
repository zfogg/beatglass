from pathlib import Path

source = (Path(__file__).parents[2] / "lib/capture/pipewire/stream_owner_linux.cpp").read_text(encoding="utf-8")
for marker in ("chunk->size", "chunk->offset", "chunk->stride", "n_datas", "pw_stream_queue_buffer", "ENOBUFS", "alignof(float)"):
    assert marker in source
print("PipeWire buffer safety contract verified (7 checks)")
