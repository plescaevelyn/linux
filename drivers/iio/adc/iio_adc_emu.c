// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

//necesara pentru stocarea datelor interne din cadrul chip-ului
struct iio_adc_emu_state {
    bool en;
    int chan0;
    int chan1;
};


struct iio_chan_spec const iio_adc_emu_chans[] = {
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
    }
};

//static pentru ca sunt functii interne
static int iio_adc_emu_write_raw (struct iio_dev *indio_dev,
	    struct iio_chan_spec const *chan,
		int val,
		int val2,
		long mask) 
{

    struct iio_adc_emu_state *st = iio_priv(indio_dev);

    switch(mask) {
        case IIO_CHAN_INFO_RAW:
            if (st->en) {
              if (chan->channel) {
                st->chan1 = val;
              }
              else {
                st->chan0 = val;
              }
              return 0;
            }
            else {
                return -EINVAL;
            }
        case IIO_CHAN_INFO_ENABLE:
            st->en = val;
            return 0;
        default:
            return -EINVAL;
    }

    return -EINVAL;
}

static int iio_adc_emu_read_raw (struct iio_dev *indio_dev,
	    struct iio_chan_spec const *chan,
		int *val,
		int *val2,
		long mask) 
{

    struct iio_adc_emu_state *st = iio_priv(indio_dev);
    
    switch(mask) {
        case IIO_CHAN_INFO_RAW:
            if(st->en) {
                if(chan->channel) {
                    //canal 1
                    *val = st->chan1;
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
        case IIO_CHAN_INFO_ENABLE:
            *val = st->en;
            return IIO_VAL_INT;            
        default:
            return -EINVAL;
    }

    return -EINVAL;
}



static const struct iio_info iio_adc_emu_info = {
   .read_raw = &iio_adc_emu_read_raw,
   .write_raw = &iio_adc_emu_write_raw,
};

static int iio_adc_emu_probe(struct spi_device *spi) {
    struct iio_dev *indio_dev;
    struct iio_adc_emu_state *st;
    //variabila care va fi returnata ca succes/esec
    //int ret;

    indio_dev = devm_iio_device_alloc(&(spi->dev), sizeof(*st));
    if (!indio_dev) {
        //cod de eroare standard de la Linux pentru cand nu se reuseste alocarea memoriei
        return -ENOMEM;
    }

    
    indio_dev->name = "iio-adc-emu";
    indio_dev->info = &iio_adc_emu_info;
    indio_dev->channels = iio_adc_emu_chans;
    indio_dev->num_channels = ARRAY_SIZE(iio_adc_emu_chans);

    st = iio_priv(indio_dev);
    st->en = 0;   
    st->chan0 = 0;
    st->chan1 = 0; 

    return devm_iio_device_register(&spi->dev, indio_dev);
}


static struct spi_driver iio_adc_emu_driver = {
    //numele driver-ului + functia cu care se probeaza
    .driver = {
        .name = "iio-adc-emu",
    },
    .probe = &iio_adc_emu_probe
};

module_spi_driver(iio_adc_emu_driver);

MODULE_AUTHOR("Volcov Sabina <volcovsabina@gmail.com>");
MODULE_DESCRIPTION("Analog Devices IIO ADC Emulator");
MODULE_LICENSE("GPL v2");