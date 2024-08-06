// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

struct ad5592r_s_state {
    bool en;
    int chan0;
    int chan1;
};

struct iio_chan_spec const ad5592r_s_chans[]={
    {
        .type = IIO_VOLTAGE,
        .indexed = 1, //daca vreau sa numerotez canalele
        .channel = 0, //numar canal
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1, //daca vreau sa numerotez canalele
        .channel = 1, //numar canal
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
    }
};

static int ad5592r_s_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val,
			int *val2,
			long mask)
{
   struct ad5592r_s_state *st = iio_priv(indio_dev);
    switch(mask){
        case IIO_CHAN_INFO_RAW:
            if(st->en){
                if(chan->channel)
                    *val=st->chan1;
                else
                    *val=st->chan0;
                return IIO_VAL_INT;
            }
            else
                return -EINVAL;
        case IIO_CHAN_INFO_ENABLE:
            *val=st->en;
            return IIO_VAL_INT;
        default:
            return -EINVAL;
    }

    return -EINVAL;  
}

static int ad5592r_s_write_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int val,
			int val2,
			long mask)
{
    struct ad5592r_s_state *st = iio_priv(indio_dev);
    switch(mask){
        case IIO_CHAN_INFO_RAW:
            if(st->en){
                if(chan->channel)
                    st->chan1=val;
                else
                    st->chan0=val;
                return 0;
            }
            else
                return -EINVAL;
        case IIO_CHAN_INFO_ENABLE:
            st->en = val;
            return 0;
        default:
            return -EINVAL;
    }

    return -EINVAL;  
}

static const struct iio_info ad5592r_s_info = {
     .read_raw=&ad5592r_s_read_raw,
     .write_raw=&ad5592r_s_write_raw,
};

static int ad5592r_s_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    struct ad5592r_s_state *st;
    // int ret;

    indio_dev = devm_iio_device_alloc(&spi->dev,sizeof(*st));
    if(!indio_dev)
        return -ENOMEM; //cod de eroare out of memory

    indio_dev->name="ad5592r_s";
    indio_dev->info = &ad5592r_s_info;
    indio_dev->channels = ad5592r_s_chans;
    indio_dev->num_channels = ARRAY_SIZE(ad5592r_s_chans);

    st = iio_priv(indio_dev);
    st->en = 0;
    st->chan0=0;
    st->chan1=0;

    return devm_iio_device_register(&spi->dev,indio_dev);
};

static struct spi_driver ad5592r_s_driver = {
    .driver = {
        .name = "ad5592r_s"
    },
    .probe = ad5592r_s_probe
};
module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Elena Hadarau <elena.hadarau@yahoo.com>");
MODULE_DESCRIPTION("Analog Devices ad5592r_s");
MODULE_LICENSE("GPL v2");