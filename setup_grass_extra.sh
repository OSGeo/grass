#!/usr/bin/env bash
set -e

echo "[+] Updating package lists..."
sudo apt update

echo "[+] Installing remaining optional GRASS dependencies..."
sudo apt install -y \
	libnetcdf-dev \
	libsvm-dev \
	libpdal-dev \
	r-base-dev \
	python3-termcolor \
	libglu1-mesa-dev libgl1-mesa-dev \
	subversion

echo "[+] Optional dependencies installed!"
echo "[+] For docs support, also run:     pip install mkdocs mkdocs-material"
