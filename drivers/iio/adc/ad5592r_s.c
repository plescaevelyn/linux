// SPDX-License-Identifier: GPL-2.0-or-later
/*
* Copyright (C) 2024 Analog Devices, Inc.
*/

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>
#include <linux/bitfield.h>
#include <asm/unaligned.h>
#include <linux/debugfs.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/buffer.h>

#define AD5592R_S_RD_BACK_EN BIT(6)
#define AD5592R_S_ADDR_RD_BACK_REG_SEL GENMASK(5, 2)

#define AD5592R_S_CONF_RD_BACK_REG 0x7

#define AD5592R_S_ADDR_WR_MSK GENMASK(14, 11)
#define AD5592R_S_VAL_WR_MSK GENMASK(9, 0)

#define AD5592R_S_REG_PD_REF BIT(9)
#define AD5592R_S_REG_CONFIG_ADC GENMASK(5, 0)

#define AD5592R_S_CONFIG_ADC_REG 0x4
#define AD5592R_S_CHAN_SEL_REG 0x2
#define AD5592R_S_REG_PD 0xB
#define AD5592R_S_RES_DATA_MSK GENMASK(11, 0)

struct ad5592r_s_state {
	struct spi_device *spi;
	bool en;
	int chan0, chan1, chan2, chan3, chan4, chan5;
};

struct iio_chan_spec const ad5592r_s_chans[] = {
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 0,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.scan_index = 0,
        .scan_type = {
            .sign = 'u',
            .realbits = 12,
            .storagebits = 16,
        }
	},

	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.scan_index = 1,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
		}

	},

	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 2,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.scan_index = 2,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
		}
	},

	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 3,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.scan_index = 3,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
		}
	},

	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 4,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.scan_index = 4,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
		}
	},

	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 5,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.scan_index = 5,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
		}
	}

};

static int ad5592r_s_nop(struct ad5592r_s_state *st, u16 *val)
{
	u16 tx = 0;
	struct spi_transfer xfer[] = { {
		.tx_buf = &tx,
		.rx_buf = val,
		.len = 2,
	} };

	return spi_sync_transfer(st->spi, xfer, 1);
}

static int ad5592r_s_spi_read(struct ad5592r_s_state *st, u8 reg, u16 *readval)
{
	u16 tx = 0;
	u16 rx = 0;
	u16 msg = 0;
	int ret;
	struct spi_transfer xfer[] = { {
		.rx_buf = NULL,
		.tx_buf = &msg,
		.len = 2,
	} };

	tx |= FIELD_PREP(AD5592R_S_ADDR_WR_MSK, AD5592R_S_CONF_RD_BACK_REG);
	tx |= AD5592R_S_RD_BACK_EN;
	tx |= FIELD_PREP(AD5592R_S_ADDR_RD_BACK_REG_SEL, reg);

	//dev_info(&st->spi->dev, "tx = 0x%x\n", tx);
	put_unaligned_be16(tx, &msg);
	//dev_info(&st->spi->dev, "msg = 0x%x\n", msg);

	ret = spi_sync_transfer(st->spi, xfer, 1);

	if (ret) {
		dev_err(&st->spi->dev, "failed spi transfer config reg");
		return ret;
	}

	ret = ad5592r_s_nop(st, &rx);
	if (ret) {
		dev_err(&st->spi->dev, "failed nop transfer");
		return ret;
	}

	*readval = get_unaligned_be16(&rx);

	return 0;
}

static int ad5592r_s_spi_write(struct ad5592r_s_state *st, u8 reg, u16 writeval)
{
	u16 tx = 0;
	u16 msg = 0;
	struct spi_transfer xfer[] = { {
		.tx_buf = &msg,
		.rx_buf = NULL,
		.len = 2,
	} };

	tx = FIELD_PREP(AD5592R_S_ADDR_WR_MSK, reg) |
	     FIELD_PREP(AD5592R_S_VAL_WR_MSK, writeval);

	//dev_info(&st->spi->dev, "tx = 0x%x\n", tx);

	put_unaligned_be16(tx, &msg);

	//dev_info(&st->spi->dev, "msg = 0x%x\n", msg);

	return spi_sync_transfer(st->spi, xfer, 1);
}

// static int ad5592r_s_read_channel_data(struct ad5592r_s_state *st,
// 				       struct iio_chan_spec const *chan,
// 				       int *readval)
// {
// 	int ret;
// 	// in probe: scrierile astea 2
// 	// scriu in 0x4: vreau canalele 00111111
// 	// scriu in 0x2: de pe ce canal vreau sa citesc


// 	ret = ad5592r_s_spi_write(st, AD5592R_S_CHAN_SEL_REG, chan->channel);
// 	if (ret) {
// 		dev_err(&st->spi->dev, "failed writing channel select reg");
// 		return ret;
// 	}

// 	u16 res;
// 	ret = ad5592r_s_nop(st, NULL);
// 	if (ret) {
// 		dev_err(&st->spi->dev, "failed nop transfer");
// 		return ret;
// 	}

// 	ret = ad5592r_s_nop(st, &res);
// 	if (ret) {
// 		dev_err(&st->spi->dev, "failed nop2 transfer");
// 		return ret;
// 	}

// 	u16 temp;
// 	temp = get_unaligned_be16(&res);

// 	*readval = temp & AD5592R_S_RES_DATA_MSK;

// 	return 0;
// }


static int ad5592r_s_read_channel_data(struct ad5592r_s_state *st,
				       struct iio_chan_spec const *chan,
				       int *readval)
{
	int ret;
	u16 res;

	// Write to the channel select register
	ret = ad5592r_s_spi_write(st, AD5592R_S_CHAN_SEL_REG, BIT(chan->channel));
	if (ret) {
		dev_err(&st->spi->dev, "Failed writing channel select reg");
		return ret;
	}

	// Perform the NOP transfer
	ret = ad5592r_s_nop(st, NULL);
	if (ret) {
		dev_err(&st->spi->dev, "Failed nop transfer");
		return ret;
	}

	ret = ad5592r_s_nop(st, &res);
	if (ret) {
		dev_err(&st->spi->dev, "Failed nop2 transfer");
		return ret;
	}

	// Directly use the received value

	u16 temp;
	temp = get_unaligned_be16(&res);
	*readval = temp & AD5592R_S_RES_DATA_MSK;

	return 0;
}


static int ad5592r_s_read_raw(struct iio_dev *indio_dev,
			      struct iio_chan_spec const *chan, int *val,
			      int *val2, long mask)
{
	struct ad5592r_s_state *st = iio_priv(indio_dev);
	int ret;
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if (st->en) {
			ret = ad5592r_s_read_channel_data(st, chan, val);
			if (ret) {
				dev_err(&st->spi->dev,
					"error reading from channel");
				return ret;
			}
			return IIO_VAL_INT;
		} else
			return -EINVAL;
	case IIO_CHAN_INFO_ENABLE:
		*val = st->en;
		return IIO_VAL_INT;

	default:
		return -EINVAL;
	}
	return -EINVAL;
};

static int ad5592r_s_write_raw(struct iio_dev *indio_dev,
			       struct iio_chan_spec const *chan, int val,
			       int val2, long mask)
{
	struct ad5592r_s_state *st = iio_priv(indio_dev);

	switch (mask) {
	// case IIO_CHAN_INFO_RAW:
	// 	if (st->en) {
	// 		switch (chan->channel) {
	// 		case 0:
	// 			st->chan0 = val;
	// 			break;
	// 		case 1:
	// 			st->chan1 = val;
	// 			break;
	// 		case 2:
	// 			st->chan2 = val;
	// 			break;
	// 		case 3:
	// 			st->chan3 = val;
	// 			break;
	// 		case 4:
	// 			st->chan4 = val;
	// 			break;
	// 		case 5:
	// 			st->chan5 = val;
	// 			break;
	// 		default:
	// 			return -EINVAL;
	// 		}

	// 		return 0;
	// 	} else
	// 		return -EINVAL;
	case IIO_CHAN_INFO_ENABLE:
		st->en = val;
		return 0;
	default:
		return -EINVAL;
	}
	return -EINVAL;
};

static irqreturn_t ad5592rs_trig_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct ad5592r_s_state *st = iio_priv(indio_dev);

	u16 buf[16];  // Adjust buffer size based on expected number of active channels
	int bit = 0;
	int i = 0;
	int ret;

	for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->masklength) 
	{
		u16 res;

		// Write the selected channel
		ret = ad5592r_s_spi_write(st, AD5592R_S_CHAN_SEL_REG, BIT(bit));
		if (ret) {
			dev_err(&st->spi->dev, "Failed writing channel select reg");
			return IRQ_HANDLED;
		}

		// Perform the NOP transfer
		ret = ad5592r_s_nop(st, NULL);
		if (ret) {
			dev_err(&st->spi->dev, "Failed nop transfer in handler");
			return IRQ_HANDLED;
		}

		ret = ad5592r_s_nop(st, &res);
		if (ret) {
			dev_err(&st->spi->dev, "Failed nop2 transfer in handler");
			return IRQ_HANDLED;
		}


		u16 temp;
		temp = get_unaligned_be16(&res);
		buf[i] = temp & AD5592R_S_RES_DATA_MSK;
		i++;
	}

	ret = iio_push_to_buffers(indio_dev, buf);
	if (ret) {
		dev_err(&st->spi->dev, "Failed to push to buffers in handler");
		return IRQ_HANDLED;
	}

	iio_trigger_notify_done(indio_dev->trig);

	return IRQ_HANDLED;
}


static int ad5592r_s_debugfs(struct iio_dev *indio_dev, unsigned reg,
			     unsigned writeval, unsigned *readval)
{
	struct ad5592r_s_state *st = iio_priv(indio_dev);

	if (readval) {
		return ad5592r_s_spi_read(st, reg, (u16 *)readval);
	}

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
	int ret;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
	if (!indio_dev)
		return -ENOMEM;

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
	st->spi = spi;

	ret = devm_iio_triggered_buffer_setup_ext(&spi->dev, indio_dev, NULL, ad5592rs_trig_handler, IIO_BUFFER_DIRECTION_IN, NULL, NULL);


	ret = ad5592r_s_spi_write(st, AD5592R_S_CONFIG_ADC_REG,
				  AD5592R_S_REG_CONFIG_ADC);

	if (ret) {
		dev_err(&st->spi->dev, "failed reg write: adc reg config");
		return ret;
	}

	// scriu in 0xB: BIT(9)

	ret = ad5592r_s_spi_write(st, AD5592R_S_REG_PD, AD5592R_S_REG_PD_REF);

	if (ret) {
		dev_err(&st->spi->dev, "failed reg write: vref on");
		return ret;
	}

	if (ret) {
		dev_err(&spi->dev, "failed writing ___ reg");
		return ret;
	}

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592r_s_driver = {
    .driver = {
        .name = "ad5592r_s",
    },
    .probe = ad5592r_s_probe
};
module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Nechita Florina-Elena <nechitaflorina2002@gmail.com>");
MODULE_DESCRIPTION("Analog Devices ADC ad5592r_s");
MODULE_LICENSE("GPL v2");