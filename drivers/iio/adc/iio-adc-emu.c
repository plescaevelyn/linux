// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <linux/spi/spi.h>
#include <linux/module.h>

#include <linux/iio/iio.h>

static int iio_adc_emu_read_raw(struct iio_dev* indio_dev, 
                                struct iio_chan_spec const *chan, 
                                int* val,
                                int* val2,
                                long mask)
{
    switch (mask)
    {
        case IIO_CHAN_INFO_ENABLE:
        {
            *val = 0;

            return IIO_VAL_INT;
        }
        case IIO_CHAN_INFO_RAW:
        {
            if (chan->channel == 1)
            {
                *val = 87;
            }
            else
            {
                *val = 420;
            }

            return IIO_VAL_INT;
        }
        default:
        {
            return -EINVAL;
        }
    }

    return -EINVAL;
}

static int iio_adc_emu_write_raw(struct iio_dev* indio_dev,
                                 struct iio_chan_spec const *chan,
                                 int val,
                                 int val2,
                                 long mask)
{
    switch (mask)
    {
        case IIO_CHAN_INFO_ENABLE:
        {
            return 0;
        }
    }

    return -EINVAL;
}

static const struct iio_info iio_adc_emu_info = {
    .read_raw = &iio_adc_emu_read_raw,
    //.write_raw = &iio_adc_emu_write_raw,
};

static const struct iio_chan_spec iio_adc_emu_channels[] = {
    {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        //.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        //.output = 0,
        .indexed = 1,
        .channel = 0,
    }, 
    {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        //.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        //.output = 0,
        .indexed = 1,
        .channel = 1,
    }
};

static int iio_adc_emu_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    // int ret;

    indio_dev = devm_iio_device_alloc(&spi->dev, 0);
    if (!indio_dev)
    {
        return -ENOMEM;
    }

    indio_dev->name = "iio-adc-emu";
    indio_dev->info = &iio_adc_emu_info;
    indio_dev->channels = iio_adc_emu_channels;
    indio_dev->num_channels = ARRAY_SIZE(iio_adc_emu_channels);

    return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver iio_adc_emu_driver = {
    .driver = {
        .name = "iio-adc-emu",
    },
    .probe = iio_adc_emu_probe,
};

module_spi_driver(iio_adc_emu_driver);

MODULE_AUTHOR("Cristea Tudor");
MODULE_DESCRIPTION("Analog Devices ADC Emulator");
MODULE_LICENSE("GPL v2");