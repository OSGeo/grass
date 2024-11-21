#!/usr/bin/env bash
# Author: Ivan Mincik, ivan.mincik@gmail.com

set -e

# build dependencies
sudo add-apt-repository ppa:ubuntugis/ubuntugis-unstable -y
sudo apt-get update -qq
