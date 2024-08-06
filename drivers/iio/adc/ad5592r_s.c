// SPDX-License-Identifier: GPL-2.0+
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
    int chan2;
    int chan3;
    int chan4;
    int chan5;
};

struct iio_chan_spec const ad5592r_s_chans[] = {
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
static int ad5592r_s_write_raw (struct iio_dev *indio_dev,
	    struct iio_chan_spec const *chan,
		int val,
		int val2,
		long mask) 
{

    struct ad5592r_s_state *st = iio_priv(indio_dev);

    switch(mask) {
        case IIO_CHAN_INFO_ENABLE:
            st->en = val;
            return 0;
        case IIO_CHAN_INFO_RAW:
            if (st->en) {
                if(chan->channel == 1) {
                    //canal 1
                    st->chan1 = val;
                }
                else if(chan->channel == 2) {
                    //canal 2
                    st->chan2 = val;
                }
                else if(chan->channel == 3) {
                    //canal 3
                    st->chan3 = val;
                }
                else if(chan->channel == 4) {
                    //canal 4
                    st->chan4 = val;
                }
                else if(chan->channel == 5) {
                    //canal 5
                    st->chan5 = val;
                }
                else {
                    //canal 0
                    st->chan0 = val;
                }
                return 0;
            }
            else {
                return -EINVAL;
            }
            
        default:
            return -EINVAL;
    }

    return -EINVAL;
}

static int ad5592r_s_read_raw (struct iio_dev *indio_dev,
	    struct iio_chan_spec const *chan,
		int *val,
		int *val2,
		long mask) 
{

    struct ad5592r_s_state *st = iio_priv(indio_dev);

    switch(mask) {
        case IIO_CHAN_INFO_ENABLE:
            *val = st->en;
            return IIO_VAL_INT;
        case IIO_CHAN_INFO_RAW:
            if (st->en) {
                if(chan->channel == 1) {
                    //canal 1
                    *val = st->chan1;
                }
                else if(chan->channel == 2) {
                    //canal 2
                    *val = st->chan2;
                }
                else if(chan->channel == 3) {
                    //canal 3
                    *val = st->chan3;
                }
                else if(chan->channel == 4) {
                    //canal 4
                    *val = st->chan4;
                }
                else if(chan->channel == 5) {
                    //canal 5
                    *val = st->chan5;
                }
                else {
                    //canal 0
                    *val = st->chan0;
                }
                return IIO_VAL_INT;
            }
            else {
                return -EINVAL;
            }
            
        default:
            return -EINVAL;
    }

    return -EINVAL;
}

static const struct iio_info ad5592r_s_info = {
    .read_raw = &ad5592r_s_read_raw,
    .write_raw = &ad5592r_s_write_raw,
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

    struct ad5592r_s_state *st = iio_priv(indio_dev);

    indio_dev->name = "ad5592r_s";
    indio_dev->info = &ad5592r_s_info;
    indio_dev->channels = ad5592r_s_chans;
    indio_dev->num_channels = ARRAY_SIZE(ad5592r_s_chans);

    st = iio_priv(indio_dev);
    st->en = 0;   
    st->chan0 = 0;
    st->chan1 = 0; 
    st->chan2 = 0;
    st->chan3 = 0; 
    st->chan4 = 0;
    st->chan5 = 0; 

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