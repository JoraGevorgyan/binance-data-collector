FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        clang \
        ccache \
        cmake \
        ninja-build \
        git \
        perl \
        pkg-config \
        python3 \
        python3-venv \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:${PATH}"
RUN pip3 install --no-cache-dir --upgrade pip3 \
    && pip3 install --no-cache-dir conan

WORKDIR /workspace
COPY . .

RUN conan profile detect --force \
    && conan install . --build=missing -s build_type=Release \
    && cmake --preset release \
    && cmake --build --preset release

CMD ["/bin/bash"]
