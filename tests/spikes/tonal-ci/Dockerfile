# Experimental CI recipe; Ubuntu transitive packages are not snapshot-pinned.
FROM ubuntu:24.04@sha256:fe81560990fae86dc4743202b2cb4d3df98d0bcf949173cecef15ce1127ef852
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential git python3 pkg-config libeigen3-dev=3.4.0-4build0.1 ca-certificates \
    && rm -rf /var/lib/apt/lists/*
RUN git clone https://github.com/MTG/essentia.git /opt/essentia && \
    git -C /opt/essentia checkout 23ae7baddecfc3227ba23ad69fa11f73ac5293a4
WORKDIR /opt/essentia
RUN python3 waf configure --lightweight= --fft=KISS --build-static --ignore-algos=Spline,CubicSpline && \
    python3 waf -j4
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends cmake libaubio-dev libsndfile1-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /work
COPY CMakeLists.txt ./
COPY include ./include
COPY lib ./lib
COPY src ./src
COPY tests ./tests
RUN cmake -S . -B /tmp/build -DBUILD_TESTING=ON -DBEATGLASS_BUILD_TONAL_SPIKE=ON -DBEATGLASS_BUILD_SLICE=ON -DBEATGLASS_TEST_V1_CODECS=OFF -DBEATGLASS_BUILD_MUSIC_REPLAY=ON \
    -DESSENTIA_INCLUDE_DIR=/opt/essentia/src/essentia \
    -DESSENTIA_LIBRARY=/opt/essentia/build/src/libessentia.a \
    -DCMAKE_CXX_FLAGS=-I/opt/essentia/src && \
    cmake --build /tmp/build -j2 && ctest --test-dir /tmp/build --output-on-failure
