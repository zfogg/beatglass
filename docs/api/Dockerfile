FROM ubuntu:24.04@sha256:fe81560990fae86dc4743202b2cb4d3df98d0bcf949173cecef15ce1127ef852
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends clang-20 python3=3.12.3-0ubuntu2.1 python3-venv python3-pip pkg-config=1.8.1-2build1 qt6-base-dev=6.4.2+dfsg-21.1build5 ca-certificates && rm -rf /var/lib/apt/lists/*
COPY docs/api/requirements.txt /tmp/requirements.txt
RUN python3 -m venv /opt/docs && /opt/docs/bin/pip install --no-cache-dir -r /tmp/requirements.txt
ENV PATH="/opt/docs/bin:${PATH}"
WORKDIR /work
