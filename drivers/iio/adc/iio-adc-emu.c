// SPDX-License-Identifier: GPL-2.0-only
/*
 * Analog Devices AD9467 SPI ADC driver
 *
 * Copyright 2024 Analog Devices Inc.
 */
#include<linux/spi/spi.h>
#include<linux/module.h>
#include<linux/iio/iio.h>

static const struct iio_info iio_adc_emu_info={
  
};
static int iio_adc_emu_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    int ret;
    indio_dev= devm_iio_device_alloc(&spi->dev, 0);
    if(!indio_dev)
         return -ENOMEM;
    indio_dev->name ="iio-adc-emu";
    indio_dev->info = &iio_adc_emu_info;
    return devm_iio_device_register(&spi->dev, indio_dev);
}
static struct spi_driver iio_adc_emu_driver={
    .driver={
        .name= "iio-adc-emu"
    },
    .probe= iio_adc_emu_probe
};

module_spi_driver(iio_adc_emu_driver);
MODULE_AUTHOR("Rosca David-Sorin ");
MODULE_DESCRIPTION("Analog Devices IIO ADC driver");
MODULE_IMPORT_NS(IIO_ADI_AXI);
