// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

struct iio_chan_spec const ad5592r_s_chans[] = {
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
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 2,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 3,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 4,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 5,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    }
    //8 canale in datasheet, dar se folosesc 6
    /*
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 6,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 7,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    }
    */
};

//static pentru ca sunt functii interne
static int ad5592r_s_read_raw (struct iio_dev *indio_dev,
	    struct iio_chan_spec const *chan,
		int *val,
		int *val2,
		long mask) 
{

    switch(mask) {
        case IIO_CHAN_INFO_RAW:
            if(chan->channel == 1) {
                //canal 1
                *val = 87;
            }
            else if(chan->channel == 2) {
                //canal 2
                *val = 65;
            }
            else if(chan->channel == 3) {
                //canal 3
                *val = 32;
            }
            else if(chan->channel == 4) {
                //canal 4
                *val = 41;
            }
            else if(chan->channel == 5) {
                //canal 5
                *val = 28;
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

static const struct iio_info ad5592r_s_info = {
    .read_raw = &ad5592r_s_read_raw,
};

static int ad5592r_s_probe(struct spi_device *spi) {
    struct iio_dev *indio_dev;
    //variabila care va fi returnata ca succes/esec
    //int ret;

    indio_dev = devm_iio_device_alloc(&(spi->dev), 0);
    if (!indio_dev) {
        //cod de eroare standard de la Linux pentru cand nu se reuseste alocarea memoriei
        return -ENOMEM;
    }


    indio_dev->name = "ad5592r_s";
    indio_dev->info = &ad5592r_s_info;
    indio_dev->channels = ad5592r_s_chans;
    indio_dev->num_channels = ARRAY_SIZE(ad5592r_s_chans);

    return devm_iio_device_register(&spi->dev, indio_dev);
}


static struct spi_driver ad5592r_s_driver = {
    //numele driver-ului + functia cu care se probeaza
    .driver = {
        .name = "ad5592r_s",
    },
    .probe = &ad5592r_s_probe
};

module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Volcov Sabina <volcovsabina@gmail.com>");
MODULE_DESCRIPTION("Analog Devices AD5592R");
MODULE_LICENSE("GPL v2");