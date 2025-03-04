#!/bin/bash

make

sudo insmod list_dev.ko
sudo rmmod list_dev
sudo dmesg | tail -10

make clean