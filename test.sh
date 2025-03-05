#!/bin/bash

make

sudo insmod list_dev.ko
sudo chmod 666 /dev/linked_list_dev

for i in {100..1}
do
    echo "ADDF $i" > /dev/linked_list_dev
done
cat /dev/linked_list_dev

sudo rmmod list_dev
make clean