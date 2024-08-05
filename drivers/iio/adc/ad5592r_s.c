// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

static const struct iio_info ad5592r_s_info = {

};

 static int ad5592r_s_probe(struct spi_device *spi)
 {
    struct iio_dev *indio_dev;
    //int ret;

    indio_dev = devm_iio_device_alloc(&spi->dev, 0);
    if(!indio_dev)
        return -ENOMEM; //Error -> No memory
    indio_dev->name = "ad5592r_s";
    indio_dev->info = &ad5592r_s_info;
    
    return devm_iio_device_register(&spi->dev, indio_dev);
 }

 static struct spi_driver ad5592r_s_driver = {
    .driver = {
        .name = "ad5592r_s",
    },
    .probe = ad5592r_s_probe
 };

 module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Risteiu Ioana");
MODULE_DESCRIPTION("Analog Devices AD5592R_S");
MODULE_LICENSE("GPL v2");