#!/usr/bin/env bash

# -----------------------------------------------------------
# Auto Install Script for Debain Bookworm (Debian 12) and/or Debian-Based Systems.
#
# Author: Anirban Das
# ----------------------------------------------------------

set -e

echo "[+] Updating package lists..."
sudo apt update

echo "[+] Installing core build tools..."
sudo apt install -y \
	build-essential cmake gcc g++ make \
	bison flex pkg-config gettext git \
	python3 python3-dev python3-pip

echo "[+] Installing core GRASS dependencies..."
sudo apt install -y \
	libproj-dev libgdal-dev libgeos-dev \
	libcairo2-dev libfreetype-dev libfontconfig1-dev \
	libpng-dev libtiff-dev zlib1g-dev \
	libbz2-dev libzstd-dev

echo "[+] Installing optional math libraries..."
sudo apt install -y \
	libfftw3-dev liblapacke-dev liblapack-dev libblas-dev libopenblas-dev

echo "[+] Installing optional database backends..."
sudo apt install -y \
	libsqlite3-dev libpq-dev \
	default-libmysqlclient-dev \
	unixodbc-dev

echo "[+] Installing optional GUI + Python support..."
sudo apt install -y \
	libwxgtk3.2-dev \
	python3-numpy python3-matplotlib python3-pil python3-dateutil \
	ffmpeg

echo "[+] All dependencies installed!"

echo "[+] Building GRASS GIS..."
mkdir -p build && cd build
cmake ..
make -j"$(nproc)"

echo "[+] Installing GRASS GIS..."
sudo make install

echo "[+] ===== GRASS GIS build complete! Run it with: grass --version ===== "
