# Build environment for lithrez and LithRez Studio: clang, CMake, Ninja and
# Qt 6, plus FFmpeg (a reference decoder for the sound tests) and Python.
#   docker build -t lithtools-rez-dev tools/lithrez/docker
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        clang lld cmake ninja-build git ca-certificates curl file \
        qt6-base-dev qt6-base-dev-tools libqt6opengl6-dev libgl-dev \
        ffmpeg python3 \
        libglx-mesa0 libgl1-mesa-dri xvfb xauth fonts-dejavu-core \
    && rm -rf /var/lib/apt/lists/*
ENV CC=clang CXX=clang++
