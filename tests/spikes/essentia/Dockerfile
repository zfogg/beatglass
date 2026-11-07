# Research-only build; transitive Ubuntu packages are not snapshot-pinned.
FROM ubuntu:24.04@sha256:fe81560990fae86dc4743202b2cb4d3df98d0bcf949173cecef15ce1127ef852
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential git python3 pkg-config libeigen3-dev=3.4.0-4build0.1 ca-certificates \
    && rm -rf /var/lib/apt/lists/*
RUN git clone https://github.com/MTG/essentia.git /opt/essentia && \
    git -C /opt/essentia checkout 23ae7baddecfc3227ba23ad69fa11f73ac5293a4
WORKDIR /opt/essentia
RUN python3 waf configure --lightweight= --fft=KISS --build-static --ignore-algos=Spline,CubicSpline && \
    python3 waf -j4
COPY tests/spikes/essentia/tonal_smoke.cpp /tmp/tonal_smoke.cpp
RUN g++ -std=c++17 $(pkg-config --cflags eigen3) -I src -I src/essentia -I src/essentia/scheduler -I src/essentia/streaming \
    -I src/essentia/streaming/algorithms -I src/essentia/utils -I build/src/essentia \
    /tmp/tonal_smoke.cpp build/src/libessentia.a -pthread -latomic -o /tmp/tonal_smoke && /tmp/tonal_smoke
