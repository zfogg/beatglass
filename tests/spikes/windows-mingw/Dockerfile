# Development-only cross-build: direct versions pinned, transitive Ubuntu packages are not.
FROM ubuntu:24.04@sha256:fe81560990fae86dc4743202b2cb4d3df98d0bcf949173cecef15ce1127ef852
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    ca-certificates git=1:2.43.0-1ubuntu7.4 python3=3.12.3-0ubuntu2.1 pkg-config=1.8.1-2build1 \
    libeigen3-dev=3.4.0-4build0.1 g++-mingw-w64-x86-64-posix=13.2.0-6ubuntu1+26.1 \
    gcc-mingw-w64-x86-64-posix=13.2.0-6ubuntu1+26.1 && rm -rf /var/lib/apt/lists/*
RUN git init /src/essentia && git -C /src/essentia fetch --depth 1 https://github.com/MTG/essentia.git 23ae7baddecfc3227ba23ad69fa11f73ac5293a4 && \
    git -C /src/essentia checkout FETCH_HEAD && \
    git init /src/aubio && git -C /src/aubio fetch --depth 1 https://github.com/aubio/aubio.git 90bd27a23123fcc524c31787c9c8fc0ae4c79378 && \
    git -C /src/aubio checkout FETCH_HEAD
WORKDIR /src/essentia
# Upstream cross-build currently hardcodes the 32-bit compiler prefix.
RUN sed -i s/i686-w64-mingw32/x86_64-w64-mingw32/g wscript && \
    python3 waf configure --cross-compile-mingw32 --lightweight= --fft=KISS --build-static \
      --include-algos=Windowing,Spectrum,SpectralPeaks,HPCP,Key,FFT,Magnitude,PeakDetection \
      --pkg-config-path=/usr/share/pkgconfig && python3 waf -j2
WORKDIR /src/aubio
RUN cp /src/essentia/waf ./waf && \
    CC=x86_64-w64-mingw32-gcc AR=x86_64-w64-mingw32-ar python3 waf configure \
      --with-target-platform=win64 --disable-tests --disable-examples --disable-fftw3 \
      --disable-sndfile --disable-avcodec --disable-samplerate --disable-wavread --disable-wavwrite && \
    python3 waf -j2 --disable-tests --disable-examples
RUN mkdir -p /out && cp /src/essentia/build/src/libessentia.a /src/essentia/src/version.h \
    /src/aubio/build/src/libaubio-5.dll /src/aubio/build/src/aubio.dll.a /out/ && \
    cp -a /src/essentia/src /out/essentia-src && cp -a /src/aubio/src /out/aubio-src && \
    cp -a /usr/include/eigen3 /out/eigen3 && \
    { printf '%s\n' 'essentia=23ae7baddecfc3227ba23ad69fa11f73ac5293a4' \
        'aubio=90bd27a23123fcc524c31787c9c8fc0ae4c79378' \
        'essentia-options=cross-compile-mingw32,lightweight,KISS,static,Windowing,Spectrum,SpectralPeaks,HPCP,Key,FFT,Magnitude,PeakDetection' \
        'essentia-patch=i686-prefix-to-x86_64' \
        'aubio-options=win64,no-tests,no-examples,no-fftw3,no-sndfile,no-avcodec,no-samplerate,no-wavread,no-wavwrite'; \
      x86_64-w64-mingw32-g++ --version | head -1; \
      dpkg-query -W gcc-mingw-w64-x86-64-posix g++-mingw-w64-x86-64-posix libeigen3-dev; } > /out/BUILD-CONFIG.txt && \
    cd /out && sha256sum libessentia.a version.h libaubio-5.dll aubio.dll.a BUILD-CONFIG.txt > CANDIDATE-SHA256.txt
