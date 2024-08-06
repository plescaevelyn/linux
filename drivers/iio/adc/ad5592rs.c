// SPDX-License-Identifier: GPL-2.0
//Copyright (C) 2024 Analog Devices, Inc.
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

struct iio_ad5592rs_state {
	bool en;
    int chan0, chan1, chan2, chan3, chan4, chan5;
};
//////////////////////////////////////////////////
struct iio_chan_spec const iio_ad5592rs_chans[] = {
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

};

static int iio_ad5592rs_read_raw(struct iio_dev *indio_dev,
				 struct iio_chan_spec const *chan, int *val,
				 int *val2, long mask)
{
	struct iio_ad5592rs_state *st = iio_priv(indio_dev);
	switch (mask) 
    {
	    case IIO_CHAN_INFO_RAW: 
        {
		    if (st -> en) 
            {
				switch (chan->channel) 
           		 {
			    case 0:
			    	*val = st -> chan0;
			    	break;
			    case 1:
				    *val = st -> chan1;
				    break;
			    case 2:
				    *val = st -> chan2;
				    break;
			    case 3:
			    	*val = st -> chan3;
				    break;
			    case 4:
				    *val = st -> chan4;
			    	break;
			    case 5:
			    	*val = st -> chan5;
				    break;
		        default:
			       *val = -1;
			        break;
           		 }
				return IIO_VAL_INT;
		    } 
            else
			    return -EINVAL;
		}
		case IIO_CHAN_INFO_ENABLE:
			*val = st -> en;
			return IIO_VAL_INT;
		default:
			return -EINVAL;
	}
	return -EINVAL;
}
//////////////////////////////////////////////

static int iio_ad5592rs_write_raw(struct iio_dev *indio_dev,
				 struct iio_chan_spec const *chan, int val,
				 int val2, long mask)
{
	struct iio_ad5592rs_state *st = iio_priv(indio_dev);
	switch(mask)
	{
	    case IIO_CHAN_INFO_ENABLE:
		    st -> en = val;
		    return 0;
		case IIO_CHAN_INFO_RAW: 
        {
		    if (st->en) 
            {
				switch (chan->channel) 
           		 {
			    case 0:
			    	st -> chan0 = val;
			    	break;
			    case 1:
				    st -> chan1 = val;
				    break;
			    case 2:
				    st -> chan2 = val;
				    break;
			    case 3:
			    	st -> chan3 = val;
				    break;
			    case 4:
				    st -> chan4 = val;
			    	break;
			    case 5:
			    	st -> chan5 = val;
				    break;
		        default:
			       	return -EINVAL;
           		 }
				return 0;
		    } 
            else
			    return -EINVAL;
		}
	    default:
		    return -EINVAL;
	}
}

/////////////////////////////////////////////
static const struct iio_info ad5592rs_info = {
	.read_raw = &iio_ad5592rs_read_raw,
	.write_raw = &iio_ad5592rs_write_raw,
};

static int ad5592rs_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	struct iio_ad5592rs_state *st;
	//int ret;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(st));

	if (!indio_dev) {
		return -ENOMEM;
	};
	indio_dev->name = "ad5592rs";
	indio_dev->info = &ad5592rs_info;
	indio_dev->channels = iio_ad5592rs_chans;
	indio_dev->num_channels = ARRAY_SIZE(iio_ad5592rs_chans);

	st = iio_priv(indio_dev);
	st -> en = 0;

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592rs_driver = {

	.driver = { .name = "ad5592rs" },

	.probe = ad5592rs_probe
};

module_spi_driver(ad5592rs_driver);

MODULE_AUTHOR("Valentin Ples valentin.ples02@gmai.com");
MODULE_DESCRIPTION("Analog Devices IIO ADC");
MODULE_LICENSE("GPL V2");
