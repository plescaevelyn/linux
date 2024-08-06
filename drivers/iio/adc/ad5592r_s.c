// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

struct iio_ad5592r_s_state {
    bool en;
    int chan0;
    int chan1;
    int chan2;
    int chan3;
    int chan4;
    int chan5;
    int chan6;
    int chan7;    
};

struct iio_chan_spec const iio_ad5592r_s_chans[] = {
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 0,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
    },

    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 1,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
    },

    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 2,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
    },

    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 3,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
    },

    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 4,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
    },

    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 5,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
    },
    
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 6,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
    },
    
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 7,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
    },
};

static int iio_ad5592r_s_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val,
			int *val2,
			long mask)
{
    struct iio_ad5592r_s_state *st = iio_priv(indio_dev);
    switch (mask)
    {
        case IIO_CHAN_INFO_RAW:
            if(st->en)
            {
                switch (chan->channel)
                {
                    case 0:
                       *val = st->chan0;
                        break;
                    case 1:
                       *val = st->chan1;
                        break;
                    case 2:
                        *val = st->chan2;
                        break;
                    case 3:
                        *val = st->chan3;
                        break;
                    case 4:
                        *val = st->chan4;
                        break;
                    case 5:
                        *val = st->chan5;
                        break;
                    case 6:
                        *val = st->chan6;
                        break;
                    case 7:
                        *val = st->chan7;
                        break;   
                }
                return IIO_VAL_INT; 
            }
            else
                return -EINVAL;
        case IIO_CHAN_INFO_ENABLE:
            *val=st->en;
             return IIO_VAL_INT;
        return IIO_VAL_INT;    
        default:
            return -EINVAL;
    }
    return -EINVAL;
}

static int iio_ad5592r_s_write_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int val,
			int val2,
			long mask)
{
    struct iio_ad5592r_s_state *st = iio_priv(indio_dev);
    switch (mask)
    {
        case IIO_CHAN_INFO_ENABLE:
            st->en = val;
            return 0;
        case IIO_CHAN_INFO_RAW:
            if(st->en)
            {
                switch (chan->channel)
                {
                    case 0:
                       st->chan0 = val;
                        break;
                    case 1:
                       st->chan1 = val;
                        break;
                    case 2:
                        st->chan2 = val;
                        break;
                    case 3:
                        st->chan3 = val;
                        break;
                    case 4:
                        st->chan4 = val;
                        break;
                    case 5:
                        st->chan5 = val;
                        break;
                    case 6:
                        st->chan6 = val;
                        break;
                    case 7:
                        st->chan7 = val;
                        break;
                }
                return 0;    
            }
            else
                return -EINVAL;
        default:
            return -EINVAL;
    }
}

static const struct iio_info iio_ad5592r_s_info = {
    .read_raw = &iio_ad5592r_s_read_raw,
    .write_raw = &iio_ad5592r_s_write_raw,
}; 

static int iio_ad5592r_s_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    struct iio_ad5592r_s_state *st;
    //int ret;

    indio_dev = devm_iio_device_alloc(&spi->dev, 0);
    if(!indio_dev)
        return ENOMEM;

    indio_dev->name = "iio-ad5592r_s";
    indio_dev->info = &iio_ad5592r_s_info;
    indio_dev->channels = iio_ad5592r_s_chans;
    indio_dev->num_channels = ARRAY_SIZE(iio_ad5592r_s_chans);

    st = iio_priv(indio_dev);
    st->en = 0;
    st->chan0 = 87;
    st->chan1 = 420;
    st->chan2 = 1;
    st->chan3 = 2;
    st->chan4 = 3;
    st->chan5 = 4;
    st->chan6 = 5;
    st->chan7 = 6;
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