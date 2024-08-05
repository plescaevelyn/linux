// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

struct iio_chan_spec const iio_ad5592r_s_chans[] = {
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
static int iio_ad5592r_s_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val,
			int *val2,
			long mask)
{
    switch (mask)
    {
    case IIO_CHAN_INFO_RAW:
        if(chan->channel)
            *val =  87;
        else
            *val = 420;
        return IIO_VAL_INT;
    default:
        return -EINVAL;
    }

    return -EINVAL;
}

static const struct iio_info iio_ad5592r_s_info = {
    .read_raw = &iio_ad5592r_s_read_raw,
};

 static int iio_ad5592r_s_probe(struct spi_device *spi)
 {
    struct iio_dev *indio_dev;
    //int ret;

    indio_dev = devm_iio_device_alloc(&spi->dev, 0);
    if(!indio_dev)
        return -ENOMEM; //Error -> No memory
    indio_dev->name = "iio-adc-emu";
    indio_dev->info = &iio_ad5592r_s_info;
    indio_dev->channels = iio_ad5592r_s_chans;
    indio_dev->num_channels = ARRAY_SIZE(iio_ad5592r_s_chans);
    return devm_iio_device_register(&spi->dev, indio_dev);
 }

 static struct spi_driver ad5592r_s_driver = {
    .driver = {
        .name = "ad5592r-s",
    },
    .probe = iio_ad5592r_s_probe
 };

 module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Raluca Zaicanu");
MODULE_DESCRIPTION("Analog Devices ad5592r_s");
MODULE_LICENSE("GPL v2");