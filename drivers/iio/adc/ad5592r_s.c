// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 Analog Devices
 */

#include <linux/spi/spi.h>
#include <linux/module.h>

#include <linux/iio/iio.h>

static const struct iio_info ad5592r_s = {

}; 

static int ad5592r_s_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    int ret;

    indio_dev = devm_iio_device_alloc(&spi->dev, 0);
    if(!indio_dev)
        return -ENOMEM;
    
    indio_dev->name = "ad5592r_s";
    indio_dev->info = &ad5592r_s;

    return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver iio_adc_emu_driver = {
    .driver = {
        .name = "ad5592r_s",
    },
    .probe = ad5592r_s_probe
};

module_spi_driver(iio_adc_emu_driver);


MODULE_AUTHOR("Alexa Alexandru");
MODULE_DESCRIPTION("Analog Device IIO EMU ADC Driver");
MODULE_LICENSE("GPL v2");