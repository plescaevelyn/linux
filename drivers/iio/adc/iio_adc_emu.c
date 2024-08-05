// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>


struct iio_chan_spec const iio_adc_emu_chans[] = {
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 0,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 1,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    }
};

//static pentru ca sunt functii interne
static int iio_adc_emu_read_raw (struct iio_dev *indio_dev,
	    struct iio_chan_spec const *chan,
		int *val,
		int *val2,
		long mask) 
{

    switch(mask) {
        case IIO_CHAN_INFO_RAW:
            if(chan->channel) {
                //canal 1
                *val = 87;
            }
            else {
                //canal 0
                *val = 420;
            }
            return IIO_VAL_INT;
            
        default:
            return -EINVAL;
    }

    return -EINVAL;
}


static const struct iio_info iio_adc_emu_info = {
   .read_raw = &iio_adc_emu_read_raw,
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
    indio_dev->channels = iio_adc_emu_chans;
    indio_dev->num_channels = ARRAY_SIZE(iio_adc_emu_chans);

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