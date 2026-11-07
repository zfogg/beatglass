FROM ubuntu:24.04@sha256:fe81560990fae86dc4743202b2cb4d3df98d0bcf949173cecef15ce1127ef852
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends clang-18 python3 python3-venv python3-pip pkg-config qt6-base-dev ca-certificates && rm -rf /var/lib/apt/lists/*
COPY docs/api/requirements.txt /tmp/requirements.txt
RUN python3 -m venv /opt/docs && /opt/docs/bin/pip install --no-cache-dir -r /tmp/requirements.txt
ENV PATH="/opt/docs/bin:${PATH}"
WORKDIR /work
