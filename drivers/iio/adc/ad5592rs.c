// SPDX-License-Identifier: GPL-2.0
//Copyright (C) 2024 Analog Devices, Inc.
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>
#include <linux/bitfield.h>

#include <asm/unaligned.h>
#include <linux/iio/buffer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>

#define CHAN_NUMBERS 						6

#define AD5592RS_WR_ADDR_MSK 				GENMASK(14, 11)
#define AD5592RS_WR_VAL_MSK 				GENMASK(9, 0)

#define AD5592RS_RDB_REG_SEL       			GENMASK(5, 2)
#define AD5592RS_RDB_EN            			BIT(6)
#define AD5592RS_CONF_RDB_REG				0x7

#define AD5592RS_CONF_IN_EN					GENMASK(5,0)
#define AD5592RS_CONF_REG					0x4

#define AD5592RS_PD_REF_CTRL_REG			0xB
#define AD5592RS_PD_REF_CTRL_VAL			BIT(9)

#define AD5592RS_SEQ_CH(chan)				BIT(chan)
#define AD5592RS_SEQ_REG		     		0X2

#define AD5592RS_RD_VAL_MSK					GENMASK(11, 0)

struct ad5592rs_state {
	struct spi_device *spi;
	bool en;
	int chan0, chan1, chan2, chan3, chan4, chan5;
};

///////////////////////////////////////////////////////////

struct iio_chan_spec const ad5592rs_chans[] = {
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 0,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.scan_index = 0,
        .scan_type = {
            .sign='u',
            .realbits = 12,
            .storagebits = 16,
        },
	},
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.scan_index = 1,
        .scan_type = {
            .sign='u',
            .realbits = 12,
            .storagebits = 16,
        },
	},
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 2,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.scan_index = 2,
        .scan_type = {
            .sign='u',
            .realbits = 12,
            .storagebits = 16,
        },
	},
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 3,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.scan_index = 3,
        .scan_type = {
            .sign ='u',
            .realbits = 12,
            .storagebits = 16,
        },
	},
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 4,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.scan_index = 4,
        .scan_type = {
            .sign='u',
            .realbits = 12,
            .storagebits = 16,
        },
	},
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 5,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.scan_index = 5,
        .scan_type = {
            .sign='u',
            .realbits = 12,
            .storagebits = 16,
        },
	},

};

///////////////////////////////////////////////////////////

static int ad5592rs_write_raw(struct iio_dev *indio_dev,
			      			struct iio_chan_spec const *chan, 
							int val, int val2, long mask)
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

///////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////

static int ad5592rs_spi_write(struct ad5592rs_state *st, u8 reg, u16 writeval)
{
	u16 tx = 0;
	u16 msg = 0;
	struct spi_transfer xfer[] = {
		{
			.tx_buf = &tx,
			.rx_buf = NULL,
			.len = 2,
		},
	};
	msg =	FIELD_PREP(AD5592RS_WR_ADDR_MSK, reg) |
      		FIELD_PREP(AD5592RS_WR_VAL_MSK, writeval);

	//dev_info(&st->spi->dev, "msg = 0x%X", msg);
	put_unaligned_be16(msg, &tx);

	//dev_info(&st->spi->dev, "tx = 0x%X", tx);

	return spi_sync_transfer(st->spi, xfer, 1);
}

///////////////////////////////////////////////////////////

static int ad5592rs_spi_read(struct ad5592rs_state *st, u8 reg, u16 *readval)
{
	u16 tx = 0;
	u16 rx = 0;
	u16 msg = 0;
	int ret;
	struct spi_transfer xfer[] = {
		{
			.rx_buf = NULL,
			.tx_buf = &msg,
			.len = 2,
		},
	};
	tx |= AD5592RS_RDB_EN;
	tx |= FIELD_PREP(AD5592RS_RDB_REG_SEL, reg);
	tx |= FIELD_PREP(AD5592RS_WR_ADDR_MSK, AD5592RS_CONF_RDB_REG);
	
	//dev_info(&st->spi->dev, "tx = 0x%X", tx);
	put_unaligned_be16(tx, &msg);
	//dev_info(&st->spi->dev, "msg = 0x%X", msg);

	ret = spi_sync_transfer(st->spi, xfer, 1);

	if(ret){
		dev_err(&st->spi->dev, "skibidid spi transfer config reg");
		return ret;
	}

	ret = ad5592rs_spi_nop(st, &rx);

	if(ret){
		dev_err(&st->spi->dev, "skibidid nop transfer");
		return ret;
	}

	*readval = get_unaligned_be16(&rx);
	return 0;

}

///////////////////////////////////////////////////////////

static int ad5592rs_read_chan(struct ad5592rs_state *st,  struct iio_chan_spec *chan, int *val)
{
    int ret;
    u16 rx = 0;
	u16 msg = 0;
    ret = ad5592rs_spi_write(st, AD5592RS_SEQ_REG, AD5592RS_SEQ_CH(chan->channel));
    if(ret){
        dev_err(&st->spi->dev, "skibidi conversion en write fail");
        return ret;
    }

	ret = ad5592rs_spi_nop(st, &rx);
	if(ret){
		dev_err(&st->spi->dev, "skibidid nop transfer");
		return ret;
	}
	ret = ad5592rs_spi_nop(st, &rx);
	if(ret){
		dev_err(&st->spi->dev, "skibidid nop transfer");
		return ret;
	}

	msg = get_unaligned_be16(&rx);
	msg &= AD5592RS_RD_VAL_MSK;
    *val = msg;
    return 0;
}

///////////////////////////////////////////////////////////

static int ad5592rs_read_raw(struct iio_dev *indio_dev,
			     			struct iio_chan_spec const *chan, 
							int *val, int *val2, long mask)
{
	struct ad5592rs_state *st = iio_priv(indio_dev);
	int ret;
	switch (mask) {
		case IIO_CHAN_INFO_RAW: {
			if (st->en) {
				ret = ad5592rs_read_chan(st, chan, val);
				if (ret){
           	    	dev_err (&st->spi->dev, "sibidi error reading channel");
                	return ret;
            	}
            	return IIO_VAL_INT;
        	}
        	else 
            	return -EINVAL;
		}
        case IIO_CHAN_INFO_ENABLE:
            *val = st-> en;
            return IIO_VAL_INT;
		default:
			return -EINVAL;
	}
}

///////////////////////////////////////////////////////////

static int ad5592rs_debugfs(struct iio_dev *indio_dev, unsigned reg, unsigned writeval, unsigned *readval)
{
    struct ad5592rs_state *st = iio_priv(indio_dev);
    if(readval)
        return ad5592rs_spi_read(st, reg, (u16*)readval);

    return ad5592rs_spi_write(st, reg, writeval);


}

///////////////////////////////////////////////////////////

static irqreturn_t ad5592rs_trig_handler(int irq, void *p)
{
    struct iio_poll_func *pf = p;
    struct iio_dev *indio_dev = pf->indio_dev;
    struct ad5592rs_state *st = iio_priv(indio_dev);
	struct iio_chan_spec *chan = indio_dev->channels;
    u16 buf[CHAN_NUMBERS];
    int bit = 0;
    int ret;
	int val = 0;
    int i = 0;
    
    for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->num_channels)
    {
		ret = ad5592rs_read_chan(st, &chan[bit], &val);
		if(ret)
    	{
    	    dev_err(&st->spi->dev, "Failed READ CHAN in handler");
    	    return IRQ_HANDLED;
    	}
		buf[i++]=val;
    }

    ret = iio_push_to_buffers(indio_dev, buf);
    if(ret)
    {
        dev_err(&st->spi->dev, "Failed PUSH TO BUFFER in handler");
        return IRQ_HANDLED;
    }

    iio_trigger_notify_done(indio_dev->trig);

    return IRQ_HANDLED;

}

///////////////////////////////////////////////////////////

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
	};
	indio_dev->name = "ad5592rs";
	indio_dev->info = &ad5592rs_info;
	indio_dev->channels = ad5592rs_chans;
	indio_dev->num_channels = ARRAY_SIZE(ad5592rs_chans);

	st = iio_priv(indio_dev);
	st->en = 0;
	st->chan0 = 0;
	st->chan1 = 0;
	st->chan2 = 0;
	st->chan3 = 0;
	st->chan4 = 0;
	st->chan5 = 0;
	st->spi = spi;
	
	
	ret = ad5592rs_spi_write(st, AD5592RS_PD_REF_CTRL_REG, AD5592RS_PD_REF_CTRL_VAL);
	if(ret){
        dev_err(&spi->dev, "skibidi REF enable failed");
        return ret;
    }
	ret = ad5592rs_spi_write(st, AD5592RS_CONF_REG, AD5592RS_CONF_IN_EN); 
    if(ret){
        dev_err(&spi->dev, "skibidi input config failed");
        return ret;
    }
	ret = devm_iio_triggered_buffer_setup_ext(&spi->dev, indio_dev, NULL, ad5592rs_trig_handler, IIO_BUFFER_DIRECTION_IN, NULL, NULL);
	if(ret)
    {
        dev_err(&spi->dev, "Failed TRIG setup");
        return ret;
    }

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
