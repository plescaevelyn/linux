// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

static const struct iio_info ad5592rs_info = 
{

};

static int iio_ad5592rs_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    //int ret

    indio_dev = devm_iio_device_alloc(&spi->dev,0);
    if(!indio_dev)
        return -ENOMEM;
    
    indio_dev->name = "ad5592rs";
    indio_dev->info = &ad5592rs_info;

    return devm_iio_device_register(&spi->dev,indio_dev);
}

static struct spi_driver iio_ad5592rs_driver = {
    .driver = {
        .name = "ad5592rs",
    },
    .probe = &iio_ad5592rs_probe
};
module_spi_driver(iio_ad5592rs_driver);


MODULE_AUTHOR("Simonca Raul simonca.raul@gmail.com");
MODULE_DESCRIPTION("Analog Devices IIO ADC AD5592rs Driver");
MODULE_LICENSE("GPL v2");