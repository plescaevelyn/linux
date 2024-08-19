// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * 
 *
 * Copyright (c) 2016 Andreas Klinger <ak@it-klinger.de>
 */

#include <asm/unaligned.h>
#include <linux/bitfield.h>
#include <linux/spi/spi.h>
#include <linux/module.h>

#include <linux/iio/buffer.h>
#include <linux/iio/iio.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>

#define AD5592R_S_WR_ADDR_MSK				GENMASK(14, 11)
#define AD5592R_S_WR_VAL_MSK				GENMASK(8, 0)

#define AD5592R_S_RDB_REG_SEL				GENMASK(5, 2)
#define AD5592R_S_RDB_EN					BIT(6)

#define AD5592R_S_CONF_RDB_REG				0x7
#define AD5592R_S_CONF_ADC_PD_REF_REG		0xB
#define AD5592R_S_CONF_ADC_SEQ_REG			0x2
#define AD5592R_S_CONF_ADC_CONFIG_REG		0x4

#define AD5592R_S_REG_REF					BIT(9)
#define AD5592R_S_ADC_VAL_MSK				GENMASK(9, 0)

#define AD5592R_S_ADC_SEQ(x) 				BIT(x)

#define AD5592R_S_ADC_EN					0x3F
#define AD5592R_S_ADC_RESULT				GENMASK(11, 0)

#define IIO_ADC_EMU_REG_CHIP_ID             0x0             

struct ad5592rs_state {
	bool en;
	int chan0;
    int chan1;
    int chan2;
    int chan3;
    int chan4;
    int chan5;
    int chan6;
    int chan7;
	struct spi_device *spi;
};

struct iio_chan_spec const ad559rs_chans[] = {
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
	},
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 6,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .scan_index = 6,
        .scan_type = {
            .sign = 'u',
            .realbits = 12,
            .storagebits = 16,
        }
	},
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 7,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .scan_index = 7,
        .scan_type = {
            .sign = 'u',
            .realbits = 12,
            .storagebits = 16,
        }
	}
};

static int ad5592rs_spi_write(struct ad5592rs_state *st, u8 reg, u16 writeVal) {
	u16 tx = 0;
	u16 msg = 0;

	struct spi_transfer xfer[] = {
		{
			.tx_buf = &tx,
			.rx_buf = NULL,
			.len = 2,
		}
	};

	msg = FIELD_PREP(AD5592R_S_WR_ADDR_MSK, reg) | FIELD_PREP(AD5592R_S_WR_VAL_MSK, writeVal);
	//dev_info(&st->spi->dev, "msg:0x%x", msg);

	put_unaligned_be16(msg, &tx);
	//dev_info(&st->spi->dev, "tx:0x%x", tx);

	return spi_sync_transfer(st->spi, xfer, 1);
}

static int ad5592rs_spi_enable_ref(struct ad5592rs_state *st, u16 writeVal) {
	u16 tx = 0;
	u16 msg = 0;

	struct spi_transfer xfer[] = {
		{
			.tx_buf = &tx,
			.rx_buf = NULL,
			.len = 2,
		}
	};

	msg = FIELD_PREP(AD5592R_S_WR_ADDR_MSK, AD5592R_S_CONF_ADC_PD_REF_REG) | FIELD_PREP(AD5592R_S_ADC_VAL_MSK, writeVal);
	//dev_info(&st->spi->dev, "msg:0x%x", msg);

	put_unaligned_be16(msg, &tx);
	//dev_info(&st->spi->dev, "tx:0x%x", tx);

	return spi_sync_transfer(st->spi, xfer, 1);
}

static int ad5592rs_spi_nop(struct ad5592rs_state *st, u16 *val) 
{
	u16 tx = 0;
	struct spi_transfer xfer[] = {
		{
			.tx_buf = &tx,
			.rx_buf = val,
			.len = 2,
		}
	};

	return spi_sync_transfer(st->spi, xfer, 1);
}

static int ad5592rs_spi_read(struct ad5592rs_state *st, u8 reg, u16 *readVal) {
	u16 tx = 0, rx = 0, msg = 0;
	int ret;

	struct spi_transfer xfer[] = {
		{
			.rx_buf = NULL,
			.tx_buf = &tx,
			.len = 2,
		}
	};

	msg |= AD5592R_S_RDB_EN;
	msg |= FIELD_PREP(AD5592R_S_RDB_REG_SEL, reg);
	msg |= FIELD_PREP(AD5592R_S_WR_ADDR_MSK, AD5592R_S_CONF_RDB_REG);

	//dev_info(&st->spi->dev, "msg:0x%x", msg);

	put_unaligned_be16(msg, &tx);
	//dev_info(&st->spi->dev, "tx:0x%x", tx);

	ret = spi_sync_transfer(st->spi, xfer, 1);
	if(ret) {
		dev_err(&st->spi->dev, "Failed spi transfer config reg");
		return ret;
	}
	
	ret = ad5592rs_spi_nop(st, &rx);
	if(ret) {
		dev_err(&st->spi->dev, "Failed nop transfer");
		return ret;
	}

	*readVal = get_unaligned_be16(&rx);

	return 0;
}

static int ad5592rs_read_channel(struct ad5592rs_state *st, const struct iio_chan_spec *chan, int *val) {
	u16 rx = 0, temp = 0;
	int ret;

	ret = ad5592rs_spi_write(st, AD5592R_S_CONF_ADC_SEQ_REG, AD5592R_S_ADC_SEQ(chan->channel));
	if(ret) {
		dev_err(&st->spi->dev, "Error at writing on channel for ADC_SEQ");
		return ret;
	}

	ret = ad5592rs_spi_nop(st, &rx);
	if(ret) {
		dev_err(&st->spi->dev, "Error at first nop");
		return ret;
	}

	ret = ad5592rs_spi_nop(st, &rx);
	if(ret) {
		dev_err(&st->spi->dev, "Error at second nop");
		return ret;
	}

	temp = get_unaligned_be16(&rx);
	temp &= AD5592R_S_ADC_RESULT;
	*val = temp;
	
	return 0;
}

int ad5592rs_read_raw(struct iio_dev *indio_dev,
			      struct iio_chan_spec const *chan, int *val,
			      int *val2, long mask)
{
	struct ad5592rs_state *st = iio_priv(indio_dev);
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if (st->en) {
			ret = ad5592rs_read_channel(st, chan, val);
			if(ret) {
				dev_err(&st->spi->dev, "Error at reading channel");
				return ret;
			}
			return IIO_VAL_INT;
		} else {
			return -EINVAL;
		}
	case IIO_CHAN_INFO_ENABLE:
		*val = st->en;
		return IIO_VAL_INT;
	default:
		return -EINVAL;
	};

	return -EINVAL;
}

int ad5592rs_write_raw(struct iio_dev *indio_dev,
			       struct iio_chan_spec const *chan, int val,
			       int val2, long mask)
{
	struct ad5592rs_state *st = iio_priv(indio_dev);
	switch (mask) {
	case IIO_CHAN_INFO_ENABLE:
		st->en = val;
		return 0;
	default:
		return -EINVAL;
	}
}

static irqreturn_t iio_adc_emu_trig_handler(int irq, void *p){
    
    struct iio_poll_func *pf = p;
    struct iio_dev *indio_dev = pf->indio_dev;
    struct ad5592rs_state *st = iio_priv(indio_dev);
    u16 buf[8];
    int bit = 0;
    int ret;
    int i = 0;
    int val = 0;

    for_each_set_bit(bit,indio_dev->active_scan_mask,indio_dev->num_channels){
        
        ad5592rs_read_channel(st,&indio_dev->channels[bit],&val);
        buf[i] = val;
        i++;
    }

    ret = iio_push_to_buffers(indio_dev, buf);
    if(ret){
        dev_err(&st->spi->dev, "FAILED push to buffers");
        return IRQ_HANDLED;
    }

    iio_trigger_notify_done(indio_dev->trig);
    return IRQ_HANDLED;

}

static int ad5592rs_debugfs(struct iio_dev *indio_dev, unsigned reg,
			unsigned writeval, unsigned *readval) 
{
	struct ad5592rs_state *st = iio_priv(indio_dev);

	if (readval) {
		return ad5592rs_spi_read(st, reg, (u16 *)readval);
	}

	return ad5592rs_spi_write(st, reg, writeval);
}

static const struct iio_info ad5592rs_info = {
	.read_raw = &ad5592rs_read_raw,
	.write_raw = &ad5592rs_write_raw,
	.debugfs_reg_access = &ad5592rs_debugfs,

};

static int ad5592rs_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	struct ad5592rs_state *st;
	int ret;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
	if (!indio_dev) {
		return -ENOMEM;
	}
	indio_dev->name = "ad5592r_s";
	indio_dev->info = &ad5592rs_info;
	indio_dev->channels = ad559rs_chans;
	indio_dev->num_channels = ARRAY_SIZE(ad559rs_chans);

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
	st->spi = spi;

	ret = ad5592rs_spi_enable_ref(st, AD5592R_S_REG_REF);
	if(ret) {
		dev_err(&st->spi->dev, "Error writing at ADC_REF enable value");
		return ret;
	}

	ret = ad5592rs_spi_write(st, AD5592R_S_CONF_ADC_CONFIG_REG, AD5592R_S_ADC_EN);
	if(ret) {
		dev_err(&st->spi->dev, "Error writing at ADC enable pins");
		return ret;
	}

    ret = devm_iio_triggered_buffer_setup_ext(&spi->dev, 
                                              indio_dev,
                                              NULL,
                                              iio_adc_emu_trig_handler,
                                              IIO_BUFFER_DIRECTION_IN,
                                              NULL,
                                              NULL);
    if(ret){
        dev_err(&st->spi->dev, "Error at trigger");
		return ret;
    }

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592rs_driver = { 
	.driver = { .name = "ad5592r_s" },
	.probe = ad5592rs_probe };
module_spi_driver(ad5592rs_driver);

MODULE_AUTHOR("Maris Radu <raducumaris@gmail.com>");
MODULE_DESCRIPTION("Analog Devices IIO ADC");
MODULE_LICENSE("GPL V2");