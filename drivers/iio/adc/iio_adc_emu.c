// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

//static pentru ca sunt functii interne

static const struct iio_info iio_adc_emu_info = {

};

static int iio_adc_emu_probe(struct spi_device *spi) {
    struct iio_dev *indio_dev;
    //variabila care va fi returnata ca succes/esec
    //int ret;

    indio_dev = devm_iio_device_alloc(&(spi->dev), 0);
    if (!indio_dev) {
        //cod de eroare standard de la Linux pentru cand nu se reuseste alocarea memoriei
        return -ENOMEM;
    }


    indio_dev->name = "iio-adc-emu";
    indio_dev->info = &iio_adc_emu_info;

    return devm_iio_device_register(&spi->dev, indio_dev);
}


static struct spi_driver iio_adc_emu_driver = {
    //numele driver-ului + functia cu care se probeaza
    .driver = {
        .name = "iio-adc-emu",
    },
    .probe = &iio_adc_emu_probe
};

module_spi_driver(iio_adc_emu_driver);

MODULE_AUTHOR("Volcov Sabina <volcovsabina@gmail.com>");
MODULE_DESCRIPTION("Analog Devices IIO ADC Emulator");
MODULE_LICENSE("GPL v2");