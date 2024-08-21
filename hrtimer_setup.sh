#!/bin/bash

mkdir cinfigs
mount -t configfs none configfs
mkdir configfs/iio/triggers/hrtimer/tmr0
echo 1 > /sys/bus/iio/devices/iio\:device0/end
echo tmr0 > /sys/bus/iio/devices/iio\:device0/trigger/current_trigger
echo 1 > /sys/bus/iio/devices/iio\:device0/scan_elements/in_voltage0_en
#1000 ms 
echo 1000 > /sys/bus/iio/devices/trigger0/sampling_frequency
