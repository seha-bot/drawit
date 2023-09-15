FROM ubuntu:22.04

ENV WINE_MONO_VERSION 0.0.8
USER root

# Install some tools required for creating the image
# RUN apt-get update \
#     && apt-get install -y --no-install-recommends \
#                 curl \
#                 unzip \
#                 ca-certificates \
#                 xvfb

RUN dpkg --add-architecture i386
RUN apt-get update -qq -y

RUN apt-get install -y -qq mingw-w64 xvfb wine-stable winetricks wine32 libgl1-mesa-glx:i386

# rm -rf /var/lib/apt/lists/*

CMD bash
