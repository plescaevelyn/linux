// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

 #include <linux/spi/spi.h>
 #include <linux/module.h>

 #include <linux/iio/iio.h>

struct ad5592r_state {
    bool en;
    int chan[6];
};

static int ad5592r_read_raw(struct iio_dev* indio_dev, 
                            struct iio_chan_spec const *chan, 
                            int* val,
                            int* val2,
                            long mask)
{
    struct ad5592r_state *st = iio_priv(indio_dev);

    switch (mask)
    {
        case IIO_CHAN_INFO_ENABLE:
        {
            *val = st->en;

            return IIO_VAL_INT;
        }
        case IIO_CHAN_INFO_RAW:
        {
            if (st->en == 1)
            {
                *val = st->chan[chan->channel];
            }
            else
            {
                return -EINVAL;
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

static int ad5592r_write_raw(struct iio_dev* indio_dev,
                             struct iio_chan_spec const *chan,
                             int val,
                             int val2,
                             long mask)
{
    struct ad5592r_state *st = iio_priv(indio_dev);

    switch (mask)
    {
        case IIO_CHAN_INFO_ENABLE:
        {
            st->en = (bool)val;

            return 0;
        }
        case IIO_CHAN_INFO_RAW:
        {
            if (st->en)
            {
                st->chan[chan->channel] = val;

                return 0;
            }
            else
            {
                return -EINVAL;
            }
        }
        default:
        {
            return -EINVAL;
        }
    }

    return -EINVAL;
}

static const struct iio_info ad5592r_info = {
    .read_raw = &ad5592r_read_raw,
    .write_raw = &ad5592r_write_raw,
};

static const struct iio_chan_spec ad5592r_channels[] = {
    {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        //.output = 0,
        .indexed = 1,
        .channel = 0,
    }, 
    {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        //.output = 0,
        .indexed = 1,
        .channel = 1,
    },
    {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        //.output = 0,
        .indexed = 1,
        .channel = 2,
    },
    {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        //.output = 0,
        .indexed = 1,
        .channel = 3,
    },
    {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        //.output = 0,
        .indexed = 1,
        .channel = 4,
    },
    {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        //.output = 0,
        .indexed = 1,
        .channel = 5,
    }
};

static int ad5592r_s_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    struct ad5592r_state *st;
    // int ret;

    indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
    if (!indio_dev)
    {
        return -ENOMEM;
    }

    indio_dev->name = "ad5592r_s";
    indio_dev->info = &ad5592r_info;
    indio_dev->channels = ad5592r_channels;
    indio_dev->num_channels = ARRAY_SIZE(ad5592r_channels);

    st = iio_priv(indio_dev);
    st->en = 0;
    for (int i = 0; i < ARRAY_SIZE(ad5592r_channels); ++i)
    {
        st->chan[i] = 0;
    }

    return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592r_s_driver = {
    .driver = {
        .name = "ad5592r_s",
    },
    .probe = ad5592r_s_probe,
};

module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Cristea Tudor");
MODULE_DESCRIPTION("Analog Devices ADC AD5592R");
MODULE_LICENSE("GPL v2");