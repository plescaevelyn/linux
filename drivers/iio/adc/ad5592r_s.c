// SPDX-License-Identifier: GPL-2.0-or-later
/*
* Copyright (C) 2024 Analog Devices, Inc.
*/

#include <linux/spi/spi.h>
#include <linux/module.h>

#include <linux/iio/iio.h>

struct iio_chan_spec const iio_adc_emu_chans[]={
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


static int iio_adc_emu_read_raw(struct iio_dev *indio_dev,
        struct iio_chan_spec const *chan,
        int *val,
        int *val2,
        long mask)
        {
            switch(mask){
                case IIO_CHAN_INFO_RAW:
                    if(chan->channel)
                        *val = 87;
                    else
                        *val = 420;
                    return IIO_VAL_INT;
            default:
                return -EINVAL;
            }
            return -EINVAL;
        };


static const struct iio_info ad5592r_s_info = {
    .read_raw = &iio_adc_emu_read_raw,

};


static int ad5592r_s_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	// int ret;

	indio_dev = devm_iio_device_alloc(&spi->dev, 0);
	if (!indio_dev)
		return -ENOMEM;

	indio_dev->name = "ad5592r_s";
	indio_dev->info = &ad5592r_s_info;
    indio_dev->channels = iio_adc_emu_chans;
    indio_dev->num_channels = ARRAY_SIZE(iio_adc_emu_chans);

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592r_s_driver = {
    .driver = {
        .name = "ad5592r_s",
    },
    .probe = ad5592r_s_probe
};
module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Maris Radu-Ioan <raducumaris@gmail.com>");
MODULE_DESCRIPTION("Analog Devices ADC ad5592r_s");
MODULE_LICENSE("GPL v2");