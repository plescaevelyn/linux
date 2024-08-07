// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

struct iio_ad5592r_s_state{
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

int temp_channel_handler(struct iio_chan_spec const *chan, int val,struct iio_dev *indio_dev)
{
    struct iio_ad5592r_s_state *st = iio_priv(indio_dev);
    if(val == -1)
    {
        switch (chan->channel)
        {
        case 0:
            return st->chan0;
        case 1:
            return st->chan1;
        case 2:
            return st->chan2;
        case 3:
            return st->chan3;
        case 4:
            return st->chan4;
        case 5:
            return st->chan5;
        case 6:
            return st->chan6;
        case 7:
            return st->chan7;
        
        default:
            return -EINVAL;
        }
        return -EINVAL;
    }
    else
    {
        switch (chan->channel)
        {
        case 0:
            st->chan0 = val;
        case 1:
            st->chan1 = val;
        case 2:
            st->chan2 = val;
        case 3:
            st->chan3 = val;
        case 4:
            st->chan4 = val;
        case 5:
            st->chan5 = val;
        case 6:
            st->chan6 = val;
        case 7:
            st->chan7 = val;
        
        default:
            return -EINVAL;
        }
        return -EINVAL;
    }
}
struct iio_chan_spec const iio_ad5592r_s_chans[] = {
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 0,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE)
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 1,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE)
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 2,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE)
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 3,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE)
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 4,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE)
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 5,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE)
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 6,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE)
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 7,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE)
    },
};

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
                temp_channel_handler(chan,val,indio_dev);
                return 0;
            }
            else 
            return -EINVAL;
        default:
            return -EINVAL;
    }

    return -EINVAL;
}

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
        if(val!=-1)
        {
            *val = temp_channel_handler(chan,-1,indio_dev);
            return IIO_VAL_INT;
        }
        else
            return -EINVAL;
    }
    else
        return -EINVAL;
    case IIO_CHAN_INFO_ENABLE:
        *val = st->en;
        return IIO_VAL_INT;    
    default:
        return -EINVAL;
    }

    return -EINVAL;
}

static const struct iio_info ad5592rs_info = 
{
    .read_raw = &iio_ad5592r_s_read_raw,
    .write_raw = &iio_ad5592r_s_write_raw,
};

static int iio_ad5592rs_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    struct iio_ad5592r_s_state *st = iio_priv(indio_dev); 
    //int ret

    indio_dev = devm_iio_device_alloc(&spi->dev,sizeof(*st));
    if(!indio_dev)
        return -ENOMEM;
    
    indio_dev->name = "ad5592rs";
    indio_dev->info = &ad5592rs_info;
    indio_dev->channels = iio_ad5592r_s_chans;
    indio_dev->num_channels = ARRAY_SIZE(iio_ad5592r_s_chans);

    st = iio_priv(indio_dev);
    st->en = 0;
    st->chan0 = 0;
    st->chan1 = 0;
    st->chan2 = 0;
    st->chan3 = 0;
    st->chan4 = 0;
    st->chan5 = 0;
    st->chan6 = 0;
    st->chan7 = 0;


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