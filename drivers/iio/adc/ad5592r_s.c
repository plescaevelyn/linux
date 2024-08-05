// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * 
 *
 * Copyright (c) 2016 Andreas Klinger <ak@it-klinger.de>
 */

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

static const struct iio_info ad5592rs_info = {

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

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592rs_driver = { .driver = { .name = "ad5592r_s" },
					     .probe = ad5592rs_probe };
module_spi_driver(ad5592rs_driver);

MODULE_AUTHOR("Hojda Matei <hojdamatei019@gmail.com>");
MODULE_DESCRIPTION("Analog Devices IIO ADC");
MODULE_LICENSE("GPL V2");