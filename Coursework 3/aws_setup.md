Steps for reproducing the `HPCE-2015-GPU-Image` AMI.

Start with the image `debian-jessie-amd64-hvm-2014-12-25-13-19-ebs (ami-aae18fc2)`
on a GPU instance.

Then do:
```
sudo sed -i -e "s/main$/main contrib non-free/g" /etc/apt/sources.list

sudo apt-get -y update

sudo apt-get -y install g++ gdb make git emacs perl libtbb-dev libtbb2

sudo apt-get -y install amd-opencl-icd amd-libopencl1 amd-opencl-dev nvidia-opencl-icd

# Needed for HL headers
sudo apt-get -y install freeglut3-dev

sudo apt-get -y upgrade
```