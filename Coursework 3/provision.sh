#!/bin/bash

#NOTE: This is for Debian Jessie, and has not been propertly
# tested in Ubuntu (and appears not to work)...

sudo sed -i -e "s/contrib$/contrib non-free/g" /etc/apt/sources.list
sudo sed -i -e "s/main$/main non-free/g" /etc/apt/sources.list

sudo apt-get -y update

sudo apt-get -y install g++ gdb make git emacs perl

sudo apt-get -y install amd-opencl-icd amd-libopencl1 amd-opencl-dev

# Needed for HL headers
sudo apt-get -y install freeglut3-dev
