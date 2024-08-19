#!/bin/bash
while true;
do
    #positive
    cat /sys/bus/iio/devices/iio\:device0/in_voltage0_raw > /sys/bus/iio/devices/iio\:device2/out_count0_raw
    cat /sys/bus/iio/devices/iio\:device0/in_voltage2_raw > /sys/bus/iio/devices/iio\:device2/out_count1_raw
    cat /sys/bus/iio/devices/iio\:device0/in_voltage4_raw > /sys/bus/iio/devices/iio\:device2/out_count2_raw
    #negative
    cat /sys/bus/iio/devices/iio\:device0/in_voltage1_raw > /sys/bus/iio/devices/iio\:device2/out_count3_raw
    cat /sys/bus/iio/devices/iio\:device0/in_voltage3_raw > /sys/bus/iio/devices/iio\:device2/out_count4_raw
    cat /sys/bus/iio/devices/iio\:device0/in_voltage5_raw > /sys/bus/iio/devices/iio\:device2/out_count5_raw
done