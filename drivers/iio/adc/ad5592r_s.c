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
    },
    
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
    },
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
        switch (chan->channel)
        {
            case 0:
                *val = 420;
                break;
            case 1:
                *val = 87;
                break;
            case 2:
                *val = 35;
                break;
            case 3:
                *val = 42;
                break;
            case 4:
                *val = 7;
                break;
            case 5:
                *val = 145;
                break;
            case 6:
                *val = 99;
                break;
            case 7:
                *val = 0;
                break;

        }
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
        return ENOMEM;

    indio_dev->name = "iio-ad5592r_s";
    indio_dev->info = &iio_ad5592r_s_info;
    indio_dev->channels = iio_ad5592r_s_chans;
    indio_dev->num_channels = ARRAY_SIZE(iio_ad5592r_s_chans);

    return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver iio_ad5592r_s_driver = {
    .driver = {
        .name = "iio-ad5592r_s"
    },
    .probe = iio_ad5592r_s_probe
};

module_spi_driver(iio_ad5592r_s_driver);

MODULE_AUTHOR("Ghisa Alexandru");
MODULE_DESCRIPTION("Analog Devices IIO ADC");
MODULE_LICENSE("GPL v2");