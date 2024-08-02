// SPDX-License-Identifier: GPL-2.0-only
/*
 * Analog Devices AD9467 SPI ADC driver
 *
 * Copyright 2024 Analog Devices Inc.
 */
#include<linux/spi/spi.h>
#include<linux/module.h>
#include<linux/iio/iio.h>

static const struct iio_info iio_ad5592r_s_info={
  
};
static int iio_ad5592r_s_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    int ret;
    indio_dev= devm_iio_device_alloc(&spi->dev, 0);
    if(!indio_dev)
         return ENOMEM;
    indio_dev->name ="ad5592r_s";
    indio_dev->info = &iio_ad5592r_s_info;
    return devm_iio_device_register(&spi->dev, indio_dev);

}
static struct spi_driver iio_ad5592r_s_driver={
    .driver={
        .name="ad5592r_s"
    },
    .probe= iio_ad5592r_s_probe
};

module_spi_driver(iio_ad5592r_s_driver);

MODULE_AUTHOR("Rosca David-Sorin ");
MODULE_DESCRIPTION("Analog Devices IIO AD5592R driver");
MODULE_IMPORT_NS("GPL v2");