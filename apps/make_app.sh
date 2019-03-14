#!/bin/bash
cd $1
if make; then
cd ../..
mkdir tmp_hd
sudo mount -o loop,offset=10240 hd_oldlinux.img tmp_hd 
sleep .5
sudo cp apps/$1/$1.bin tmp_hd/root 
sudo umount tmp_hd
rmdir tmp_hd
else
echo 'GCC compile failed'
fi
