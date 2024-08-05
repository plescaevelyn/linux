// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>
int temp_channel_handler(struct iio_chan_spec const *chan)
{
    switch (chan->channel)
    {
    case 0:
        return -EINVAL;
    case 1:
        return 10;
    case 2:
        return 20;
    case 3:
        return 30;
    case 4:
        return 40;
    case 5:
        return 50;
    case 6:
        return 60;
    case 7:
        return 70;  
    
    default:
        return -EINVAL;
    }
    return -EINVAL;
}
struct iio_chan_spec const iio_ad5592r_s_chans[] = {
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 0,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW)
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 1,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW)
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 2,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW)
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 3,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW)
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 4,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW)
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 5,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW)
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 6,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW)
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 7,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW)
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
        *val = temp_channel_handler(chan);
        return IIO_VAL_INT;
    
    default:
        return -EINVAL;
    }

    return -EINVAL;
}

static const struct iio_info ad5592rs_info = 
{
    .read_raw = &iio_ad5592r_s_read_raw,
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
    indio_dev->channels = iio_ad5592r_s_chans;
    indio_dev->num_channels = ARRAY_SIZE(iio_ad5592r_s_chans);

    return devm_iio_device_register(&spi->dev,indio_dev);
}

static struct spi_driver iio_ad5592rs_driver = {
    .driver = {
        .name = "ad5592r_s",
    },
    .probe = &iio_ad5592rs_probe
};
module_spi_driver(iio_ad5592rs_driver);


MODULE_AUTHOR("Simonca Raul simonca.raul@gmail.com");
MODULE_DESCRIPTION("Analog Devices IIO ADC AD5592rs Driver");
MODULE_LICENSE("GPL v2");