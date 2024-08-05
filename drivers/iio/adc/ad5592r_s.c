// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * 
 *
 * Copyright (c) 2016 Andreas Klinger <ak@it-klinger.de>
 */

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

struct iio_chan_spec const iio_adc_ad559rs_chans[] = {
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
	}
};

int iio_adc_ad559rs_read_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan, int *val,
			     int *val2, long mask)
{
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		switch (chan->channel) {
		case 1:
			*val = 1000;
			break;
		case 2:
			*val = 2000;
			break;
		case 3:
			*val = 3000;
			break;
		case 4:
			*val = 4000;
			break;
		case 5:
			*val = 5000;
			break;
		default:
			*val = 100;
			break;
		}

		return IIO_VAL_INT;

	default:
		return -EINVAL;
	};

	return -EINVAL;
}

static const struct iio_info ad5592rs_info = {
	.read_raw = &iio_adc_ad559rs_read_raw,
};

static int ad5592rs_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	//int ret;

	indio_dev = devm_iio_device_alloc(&spi->dev, 0);
	if (!indio_dev) {
		return -ENOMEM;
	}
	indio_dev->name = "ad5592r_s";
	indio_dev->info = &ad5592rs_info;
	indio_dev->channels = iio_adc_ad559rs_chans;
	indio_dev->num_channels = ARRAY_SIZE(iio_adc_ad559rs_chans);

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592rs_driver = {
	.driver = { .name = "ad5592r_s" },
	.probe = ad5592rs_probe
};
module_spi_driver(ad5592rs_driver);

MODULE_AUTHOR("Hojda Matei <hojdamatei019@gmail.com>");
MODULE_DESCRIPTION("Analog Devices IIO ADC");
MODULE_LICENSE("GPL V2");