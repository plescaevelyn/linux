#!/bin/bash

mkdir configfs 
mount -t configfs none configfs 
systemctl daemon-reload
mkdir configfs/iio/triggers/hrtimer/tmr0
echo 1 > /sys/bus/iio/devices/iio\:device0/en 
echo tmr0 > /sys/bus/iio/devices/iio\:device0/trigger/current_trigger
echo 1000 > /sys/bus/iio/devices/trigger0/sampling_frequency
