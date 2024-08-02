// SPDX-License-Identifier: GPL-2.0-or-later
/*
* Copyright (C) 2024 Analog Devices, Inc.
*/

#include <linux/spi/spi.h>
#include <linux/module.h>

#include <linux/iio/iio.h>

struct iio_chan_spec const ad5592r_s_chans[] = {
	{
		// channel 1
		.type = IIO_VOLTAGE,
		.indexed = 1, // daca numerotam sau nu
		.channel = 0,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	},

	{
		// channel 2
		.type = IIO_VOLTAGE,
		.indexed = 1, // daca numerotam sau nu
		.channel = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	},

	{
		// channel 3
		.type = IIO_VOLTAGE,
		.indexed = 1, // daca numerotam sau nu
		.channel = 2,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	},

	{
		// channel 4
		.type = IIO_VOLTAGE,
		.indexed = 1, // daca numerotam sau nu
		.channel = 3,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	},

	{
		// channel 5
		.type = IIO_VOLTAGE,
		.indexed = 1, // daca numerotam sau nu
		.channel = 4,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	},

	{
		// channel 6
		.type = IIO_VOLTAGE,
		.indexed = 1, // daca numerotam sau nu
		.channel = 5,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	}

};

static int ad5592r_s_read_raw(struct iio_dev *indio_dev,
			      struct iio_chan_spec const *chan, int *val,
			      int *val2, long mask)
{
	switch (mask) {
	case IIO_CHAN_INFO_RAW:

		switch (chan->channel) {
		case 0:
			*val = 10;
			break;
		case 1:
			*val = 11;
			break;
		case 2:
			*val = 12;
			break;
		case 3:
			*val = 13;
			break;
		case 4:
			*val = 14;
			break;
		case 5:
			*val = 15;
			break;
		default:
			return -EINVAL;
		}

		return IIO_VAL_INT;

	default:
		return -EINVAL;
	}
	return -EINVAL;
};

static const struct iio_info ad5592r_s_info = {
	.read_raw = &ad5592r_s_read_raw,
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
	indio_dev->channels = ad5592r_s_chans;
	indio_dev->num_channels = ARRAY_SIZE(ad5592r_s_chans);

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592r_s_driver = {
    .driver = {
        .name = "ad5592r_s",
    },
    .probe = ad5592r_s_probe
};
module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Moldovan Maria <maria_moldovan_2003@yahoo.com>");
MODULE_DESCRIPTION("Analog Devices ADC ad5592r_s");
MODULE_LICENSE("GPL v2");