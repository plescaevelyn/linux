// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 * Author: Radu Sabau <radu.sabau@analog.com>
 */
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/component.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/math.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/spi/spi.h>
#include <linux/util_macros.h>
#include <linux/units.h>
#include <linux/types.h>

#include <asm/unaligned.h>

#include <linux/iio/buffer.h>
#include <linux/iio/iio.h>

#include <linux/iio/trigger.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>

#define AD5592R_S_REG_ADDR_MSK		GENMASK(14, 11)
#define AD5592R_S_WR_VAL_MSK		GENMASK(8, 0)
#define AD5592R_S_RDB_REG_SEL		GENMASK(5, 2)
#define AD5592R_S_CHAN_CONFIG_MSK	GENMASK(7, 0)
#define AD5592R_S_CHAN_DATA_CONV	GENMASK(11, 0)

#define AD5592R_S_RDB_EN		BIT(6)
#define AD5592R_S_PD_EN			BIT(9)

#define AD5592R_S_CHAN(x)		BIT(x)
#define AD5592R_S_ADC_RANGE		BIT(5)

#define AD5592R_S_GEN_CTRL_REG		0x03
#define AD5592R_S_CONF_RDB_REG		0x07
#define AD5592R_S_PD_REF_CTRL		0x0B
#define AD5592R_S_ADC_CONFIG		0x04
#define AD5592R_S_ADC_CHAN_INPUTS	0x3F
#define AD5592R_S_ADC_SEQ		0x02

#define AD5592R_S_SAMP_FREQ(freq)	DIV_ROUND_CLOSEST((freq), 64)

struct ad5592r_s_state {
	struct spi_device	*spi;

	struct iio_trigger	*trig;

	u16 			channel0;
	u16 			channel1;
	u16 			channel2;
	u16 			channel3;
	u16 			channel4;
	u16 			channel5;
	u16 			channel6;
	u16 			channel7;

	int 			vref;
	bool			en;
};

#define AD5592R_S_CHANNEL(_idx)						\
	{								\
		.type = IIO_VOLTAGE,					\
		.indexed = 1,						\
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE) |	\
					BIT(IIO_CHAN_INFO_SAMP_FREQ) |	\
					BIT(IIO_CHAN_INFO_SCALE),	\
		.channel = _idx,					\
		.scan_index = _idx,					\
		.scan_type = {						\
			.sign = 'u',					\
			.realbits = 12,					\
			.storagebits = 16,				\
		},							\
	}

static const struct iio_chan_spec ad5592r_s_channels[] = {
	AD5592R_S_CHANNEL(0),
	AD5592R_S_CHANNEL(1),
	AD5592R_S_CHANNEL(2),
	AD5592R_S_CHANNEL(3),
	AD5592R_S_CHANNEL(4),
	AD5592R_S_CHANNEL(5),
};

static int ad5592r_s_spi_nop(struct ad5592r_s_state *st, u16 *val)
{
	u16 tx = 0;
	struct spi_transfer xfer = {
		.tx_buf = &tx,
		.rx_buf = val,
		.len = 2,
	};

	return spi_sync_transfer(st->spi, &xfer, 1);
}

static int ad5592r_s_spi_write(struct ad5592r_s_state *st, u8 reg,
			       u16 writeval)
{
	u16 tx = 0;
	u16 msg = 0;
	struct spi_transfer xfer = {
		.tx_buf = &msg,
		.rx_buf = NULL,
		.len = 2
	};

	tx = FIELD_PREP(AD5592R_S_REG_ADDR_MSK, reg) |
	     FIELD_PREP(AD5592R_S_WR_VAL_MSK, writeval);

	dev_info(&st->spi->dev, "tx = 0x%X", tx);
	put_unaligned_be16(tx, &msg);
	dev_info(&st->spi->dev, "msg = 0x%X", msg);

	return spi_sync_transfer(st->spi, &xfer, 1);
}

static int ad5592r_s_spi_read(struct ad5592r_s_state *st, u8 reg, u16 *readval)
{
	u16 tx = 0;
	u16 msg = 0;
	u16 rx = 0;
	int ret;

	struct spi_transfer xfer = {
		.rx_buf = NULL,
		.tx_buf = &tx,
		.len = 2,
	};

	msg |= AD5592R_S_RDB_EN;
	msg |= FIELD_PREP(AD5592R_S_RDB_REG_SEL, reg);
	msg |= FIELD_PREP(AD5592R_S_REG_ADDR_MSK, AD5592R_S_CONF_RDB_REG);

	dev_info(&st->spi->dev, "tx = 0x%X", msg);
	put_unaligned_be16(msg, &tx);
	dev_info(&st->spi->dev, "msg = 0x%X", tx);

	ret = spi_sync_transfer(st->spi, &xfer, 1);

	if (ret) {
		dev_err(&st->spi->dev, "Failed spi transfer config reg");
		return ret;
	}

	ret = ad5592r_s_spi_nop(st, &rx);
	if (ret) {
		dev_err(&st->spi->dev, "Failed nop transfer");
		return ret;
	}
	*readval = get_unaligned_be16(&rx);

	return 0;
}

static int ad5592r_s_spi_update_bits(struct ad5592r_s_state *st, u8 reg,
				     u16 mask, u16 val)
{
	int ret;
	u16 tmp;

	ret = ad5592r_s_spi_read(st, reg, &tmp);
	if (ret < 0)
		return ret;

	tmp = tmp & ~mask;
	tmp |= val & mask;

	return ad5592r_s_spi_write(st, reg, tmp);
}

static int ad5592r_s_read_chan(struct ad5592r_s_state *st, int chan_index,
			       int *readval)
{
	u16 rx = 0;
	u16 tx = 0;
	u16 msg = 0;
	u16 data = 0;
	int ret;

	struct spi_transfer xfer = {
		.tx_buf = &tx,
		.rx_buf = NULL,
		.len = 2,
	};

	msg = FIELD_PREP(AD5592R_S_REG_ADDR_MSK, AD5592R_S_ADC_SEQ) |
	      FIELD_PREP(AD5592R_S_CHAN_CONFIG_MSK,
			 AD5592R_S_CHAN(chan_index));

	dev_info(&st->spi->dev, "READ CHAN : tx = 0x%X", msg);
	put_unaligned_be16(msg, &tx);
	dev_info(&st->spi->dev, "READ CHAN : msg = 0x%X", tx);

	ret = spi_sync_transfer(st->spi, &xfer, 1);
	if (ret) {
		dev_err(&st->spi->dev, "Failed SPI transfer config reg\n");
		return ret;
	}

	ret = ad5592r_s_spi_nop(st, &data);
	if (ret) {
		dev_err(&st->spi->dev, "Failed nop transfer\n");
		return ret;
	}

	ret = ad5592r_s_spi_nop(st, &data);
	if (ret) {
		dev_err(&st->spi->dev, "Failed nop transfer\n");
		return ret;
	}
	rx = get_unaligned_be16(&data);
	*readval = FIELD_GET(AD5592R_S_CHAN_DATA_CONV, rx);

	return 0;
}

static int ad5592r_s_read_raw(struct iio_dev *indio_dev,
			      struct iio_chan_spec const *chan, int *val,
			      int *val2, long mask)
{
	struct ad5592r_s_state *st = iio_priv(indio_dev);
	u16 reg_val;
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if (st->en) {
			ret = ad5592r_s_read_chan(st, chan->channel, val);
			if (ret) {
				dev_err(&st->spi->dev,
					"FAILED reading data from channels!\n");
				return ret;
			}
			return IIO_VAL_INT;
		} else
			return -EINVAL;
	case IIO_CHAN_INFO_ENABLE:
		*val = st->en;
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_SAMP_FREQ:
		*val = AD5592R_S_SAMP_FREQ(st->spi->max_speed_hz);

		return IIO_VAL_INT;
	case IIO_CHAN_INFO_SCALE:
		ret = ad5592r_s_spi_read(st, AD5592R_S_GEN_CTRL_REG, &reg_val);
		if (ret) {
			dev_err(&st->spi->dev, "Failed to READ register!\n");
			return ret;
		}

		if (FIELD_GET(AD5592R_S_ADC_RANGE, reg_val))
			*val = (2 * st->vref) / 1000;
		else
			*val = (st->vref) / 1000;

		*val2 = 12;

		return IIO_VAL_FRACTIONAL_LOG2;
	default:
		return -EINVAL;
	}

	return -EINVAL;
}

static int ad5592r_s_write_raw(struct iio_dev *indio_dev,
			       struct iio_chan_spec const *chan, int val,
			       int val2, long mask)
{
	struct ad5592r_s_state *st = iio_priv(indio_dev);
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if (st->en) {
			switch (chan->channel) {
			case 0:
				st->channel0 = val;
				return 0;
			case 1:
				st->channel1 = val;
				return 0;
			case 2:
				st->channel2 = val;
				return 0;
			case 3:
				st->channel3 = val;
				return 0;
			case 4:
				st->channel4 = val;
				return 0;
			case 5:
				st->channel5 = val;
				return 0;
			default:
				return -EINVAL;
			}
		} else
			return -EINVAL;
	case IIO_CHAN_INFO_ENABLE:
		st->en = val;
		return 0;
	case IIO_CHAN_INFO_SCALE:
		ret = ad5592r_s_spi_update_bits(st, AD5592R_S_GEN_CTRL_REG,
						AD5592R_S_ADC_RANGE,
						FIELD_PREP(AD5592R_S_ADC_RANGE,
							   val));
		if (ret) {
			dev_err(&st->spi->dev, "Failed setting scale!\n");
			return ret;
		}

		return 0;
	default:
		return -EINVAL;
	}

	return -EINVAL;
}

static const struct iio_trigger_ops ad5592r_s_trigger_ops = {
	.validate_device = iio_trigger_validate_own_device,
};

static irqreturn_t ad5592r_s_trig_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct ad5592r_s_state *st = iio_priv(indio_dev);
	int buf[6];
	int bit = 0;
	int ret;
	int i = 0;

	printk("triggered\n");

	for_each_set_bit(bit, indio_dev->active_scan_mask,
			 indio_dev->num_channels) {
		ret = ad5592r_s_read_chan(st, bit, &buf[i]);
		if (ret) {
			dev_err(&st->spi->dev,
				"FAILED reading data from channels!\n");
			return ret;
		}

		ret = iio_push_to_buffers_with_timestamp(indio_dev, &buf[i],
						iio_get_time_ns(indio_dev));
		if (ret) {
			dev_err(&st->spi->dev, "FAILED push to buffers!");
			return IRQ_HANDLED;
		}
		i++;
	}

	iio_trigger_notify_done(indio_dev->trig);
	return IRQ_HANDLED;
}

static int ad5592r_s_enable_power_ref(struct ad5592r_s_state *st)
{
	u16 msg = 0;
	u16 tx = 0;
	struct spi_transfer xfer = {
		.rx_buf = NULL,
		.tx_buf = &tx,
		.len = 2,
	};

	msg |= FIELD_PREP(AD5592R_S_REG_ADDR_MSK, AD5592R_S_PD_REF_CTRL) |
	       AD5592R_S_PD_EN;

	dev_info(&st->spi->dev, "tx = 0x%X", msg);
	put_unaligned_be16(msg, &tx);
	dev_info(&st->spi->dev, "msg = 0x%X", tx);

	return spi_sync_transfer(st->spi, &xfer, 1);
}

static int ad5592r_s_adc_config(struct ad5592r_s_state *st)
{
	u16 msg = 0;
	u16 tx = 0;
	struct spi_transfer xfer = {
		.rx_buf = NULL,
		.tx_buf = &tx,
		.len = 2,
	};

	msg |= FIELD_PREP(AD5592R_S_REG_ADDR_MSK, AD5592R_S_ADC_CONFIG) |
	       FIELD_PREP(AD5592R_S_CHAN_CONFIG_MSK, AD5592R_S_ADC_CHAN_INPUTS);

	dev_info(&st->spi->dev, "tx = 0x%X", msg);
	put_unaligned_be16(msg, &tx);
	dev_info(&st->spi->dev, "msg = 0x%X", tx);

	return spi_sync_transfer(st->spi, &xfer, 1);
}

static int ad5592r_s_debugfs(struct iio_dev *indio_dev, unsigned reg,
			     unsigned writeval, unsigned *readval)
{
	struct ad5592r_s_state *st = iio_priv(indio_dev);

	if (readval)
		return ad5592r_s_spi_read(st, reg, (u16 *)readval);

	return ad5592r_s_spi_write(st, reg, writeval);
}

static const struct iio_info ad5592r_s_info = {
	.read_raw = &ad5592r_s_read_raw,
	.write_raw = &ad5592r_s_write_raw,
	.debugfs_reg_access = &ad5592r_s_debugfs,
};

static int ad5592r_s_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	struct ad5592r_s_state *st;
	u32 vref;
	int ret;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
	if (!indio_dev)
		return -ENOMEM;

	st = iio_priv(indio_dev);
	st->spi = spi;
	spi_set_drvdata(spi, indio_dev);
	spi->mode = SPI_MODE_2;
	spi_setup(spi);

	st->en = 0;
	st->channel0 = 0;
	st->channel1 = 0;
	st->channel2 = 0;
	st->channel3 = 0;
	st->channel4 = 0;
	st->channel5 = 0;

	ret = ad5592r_s_enable_power_ref(st);
	if (ret) {
		dev_err(&spi->dev, "FAILED disable external power reference!");

		return ret;
	}

	ret = ad5592r_s_adc_config(st);
	if (ret) {
		dev_err(&spi->dev, "FAILED configuring ADC inputs!");

		return ret;
	}

	indio_dev->name = "ad5592r_s";
	indio_dev->info = &ad5592r_s_info;
	indio_dev->channels = ad5592r_s_channels;
	indio_dev->num_channels = ARRAY_SIZE(ad5592r_s_channels);
	indio_dev->modes = INDIO_DIRECT_MODE;

	ret = device_property_read_u32(&spi->dev, "adi,vref", &vref);
	if (ret) {
		dev_err(&spi->dev, "FAILED to read reference voltage!\n");

		return ret;
	}
	st->vref = (int)vref;

	st->trig = devm_iio_trigger_alloc(&spi->dev, "%s-dev%d",
					  indio_dev->name,
					  iio_device_id(indio_dev));
	if (!st->trig)
		return -ENOMEM;

	st->trig->ops = &ad5592r_s_trigger_ops;
	iio_trigger_set_drvdata(st->trig, st);
	ret = devm_iio_trigger_register(&spi->dev, st->trig);
	if (ret) {
		dev_err(&spi->dev, "IIO trigger register failed\n");
		return ret;
	}

	indio_dev->trig = iio_trigger_get(st->trig);

	ret = devm_iio_triggered_buffer_setup(&spi->dev, indio_dev,
					      NULL, &ad5592r_s_trig_handler,
					      NULL);
	if (ret) {
		dev_err(&spi->dev, "IIO triggered buffer setup failed\n");
		return ret;
	}

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static const struct of_device_id ad5592r_s_of_match[] = {
	{ .compatible = "ad5592r_s", },
	{},
};
MODULE_DEVICE_TABLE(of, ad5592r_s_of_match);

static struct spi_driver ad5592r_s_driver = {
	.driver = {
		.name = "ad5592r_s",
		.of_match_table = ad5592r_s_of_match,
	},
	.probe = ad5592r_s_probe,
};
module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Radu Sabau <radu.sabau@analog.com>");
MODULE_DESCRIPTION("AD5592R_S ADC Driver");
MODULE_LICENSE("GPL v2");
