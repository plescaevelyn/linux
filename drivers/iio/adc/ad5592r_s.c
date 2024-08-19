#include <asm/unaligned.h>

#include <linux/bitfield.h>
#include <linux/module.h>

#include <linux/spi/spi.h>

#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>


#define AD5592R_S_WR_ADDR_MASK              GENMASK(14, 11)
#define AD5592R_S_WR_VALUE_MASK             GENMASK(8, 0)

#define AD5592R_S_RDBK_REG_SEL_MASK         GENMASK(5, 2)
#define AD5592R_S_RDBK_EN_MASK              BIT(6)

#define AD5592R_S_CONF_RDBK_REG_ADDR        0x7

#define AD5592R_S_RDBK_VALUE_MASK           GENMASK(11, 0)

#define AD5592R_S_ADC_SEQ_REG_ADDR          0x2
#define AD5592R_S_ADC_CONFIG_REG_ADDR       0x4

#define AD5592R_S_CONV_CHANNEL(x)           BIT(x)
#define AD5592R_S_STOP_CONV_CHANNELS        0x0
#define AD5592R_S_CONFIG_ADC_CHANNELS       GENMASK(7,0)

#define AD5592R_S_ADC_DATA_MASK             GENMASK(11,0)
#define AD5592R_S_ADC_CHANNEL_MASK          GENMASK(14, 12)

#define AD5592R_S_VREF_REG_ADDR             0xB
#define AD5592R_S_VREF_EN                   BIT(9)

struct ad5592r_s_state {
	struct spi_device *spi;
	bool enable;
	u16 channel0;
	u16 channel1;
	u16 channel2;
	u16 channel3;
	u16 channel4;
	u16 channel5;
	u16 channel6;
	u16 channel7;
};

static const struct iio_chan_spec ad5592r_s_channels[] = {
	{
		.type = IIO_VOLTAGE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.indexed = 1,
		.channel = 0,
        .scan_index = 0,
        .scan_type = {
            .sign = 'u',
            .realbits = 12,
            .storagebits = 16,
            //.endianness = IIO_BE,
        }
	},
	{
		.type = IIO_VOLTAGE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.indexed = 1,
		.channel = 1,
        .scan_index = 1,
        .scan_type = {
            .sign = 'u',
            .realbits = 12,
            .storagebits = 16,
            //.endianness = IIO_BE,
        }
	},
	{
		.type = IIO_VOLTAGE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.indexed = 1,
		.channel = 2,
        .scan_index = 2,
        .scan_type = {
            .sign = 'u',
            .realbits = 12,
            .storagebits = 16,
            //.endianness = IIO_BE,
        }
	},
	{
		.type = IIO_VOLTAGE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.indexed = 1,
		.channel = 3,
        .scan_index = 3,
        .scan_type = {
            .sign = 'u',
            .realbits = 12,
            .storagebits = 16,
            //.endianness = IIO_BE,
        }
	},
	{
		.type = IIO_VOLTAGE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.indexed = 1,
		.channel = 4,
        .scan_index = 4,
        .scan_type = {
            .sign = 'u',
            .realbits = 12,
            .storagebits = 16,
            //.endianness = IIO_BE,
        }
	},
	{
		.type = IIO_VOLTAGE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.indexed = 1,
		.channel = 5,
        .scan_index = 5,
        .scan_type = {
            .sign = 'u',
            .realbits = 12,
            .storagebits = 16,
            //.endianness = IIO_BE,
        }
	},
	{
		.type = IIO_VOLTAGE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.indexed = 1,
		.channel = 6,
        .scan_index = 6,
        .scan_type = {
            .sign = 'u',
            .realbits = 12,
            .storagebits = 16,
            //.endianness = IIO_BE,
        }
	},
	{
		.type = IIO_VOLTAGE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.indexed = 1,
		.channel = 7,
        .scan_index = 7,
        .scan_type = {
            .sign = 'u',
            .realbits = 12,
            .storagebits = 16,
            //.endianness = IIO_BE,
        }
	}
};

static int ad5592r_s_spi_noop(struct ad5592r_s_state *state, u16 *ret_val)
{
	u16 tx = 0;
    *ret_val = 0;
	struct spi_transfer xfer[] = { {
		.rx_buf = ret_val,
		.tx_buf = &tx,
		.len = 2,
	} };

	int ret = spi_sync_transfer(state->spi, xfer, 1);
    //dev_info(&state->spi->dev, "SPI_NOOP: read tx: %X", *ret_val);

    return ret;
}

static int ad5592r_s_spi_read(struct ad5592r_s_state *state, u8 reg_addr,
			      u16 *read_val)
{
	u16 tx = 0;
	u16 rx = 0;
	u16 msg = 0;
	struct spi_transfer xfer[] = {
		{
			.rx_buf = NULL,
			.tx_buf = &tx,
			.len = 2,
		},
	};

	msg |= FIELD_PREP(AD5592R_S_WR_ADDR_MASK, AD5592R_S_CONF_RDBK_REG_ADDR);
	msg |= AD5592R_S_RDBK_EN_MASK;
	msg |= FIELD_PREP(AD5592R_S_RDBK_REG_SEL_MASK, reg_addr);
	put_unaligned_be16(msg, &tx);
	//dev_info(&state->spi->dev, "SPI read msg: %X", msg);
	//dev_info(&state->spi->dev, "SPI read tx %X", tx);

	int ret = spi_sync_transfer(state->spi, xfer, 1);
	if (ret) {
		dev_err(&state->spi->dev, "SPI transfer failed on config reg");
		return ret;
	}

	ret = ad5592r_s_spi_noop(state, &rx);
	if (ret) {
		dev_err(&state->spi->dev, "SPI sync transfer failed on noop");
		return ret;
	}
	*read_val = get_unaligned_be16(&rx);
	//dev_info(&state->spi->dev, "SPI read rx %X", rx);
	//dev_info(&state->spi->dev, "SPI read read_val %X", *read_val);

	return 0;
}

static int ad5592r_s_spi_write(struct ad5592r_s_state *state, u8 reg_addr,
			       u16 write_val)
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

	msg |= FIELD_PREP(AD5592R_S_WR_ADDR_MASK, reg_addr);
	msg |= FIELD_PREP(AD5592R_S_WR_VALUE_MASK, write_val);
	put_unaligned_be16(msg, &tx);
	//dev_info(&state->spi->dev, "SPI write msg: %X", msg);
	//dev_info(&state->spi->dev, "SPI write tx: %X", tx);

	int ret = spi_sync_transfer(state->spi, xfer, 1);
	if (ret) {
		dev_err(&state->spi->dev, "SPI sync transfer failed on write");
		return ret;
	}

	return 0;
}

static int ad5592r_s_read_channel(struct iio_dev *indio_dev,
				  struct iio_chan_spec const *chan, int *read_val)
{
    u16 rx = 0;
    u16 tx = 0;
    u16 msg = 0;
    int ret = 0;
	struct spi_transfer xfer[] = {
		{
			.tx_buf = &tx,
			.rx_buf = NULL,
			.len = 2,
		},
	};

    struct ad5592r_s_state *state = iio_priv(indio_dev);

    msg = 0;
    msg |= FIELD_PREP(AD5592R_S_WR_ADDR_MASK, AD5592R_S_ADC_SEQ_REG_ADDR);
	msg |= FIELD_PREP(AD5592R_S_WR_VALUE_MASK, AD5592R_S_CONV_CHANNEL(chan->channel));
    put_unaligned_be16(msg, &tx);
	//dev_info(&state->spi->dev, "READ_CHANNEL: write ADC_SEQ START tx: %X", tx);
    ret = spi_sync_transfer(state->spi, xfer, 1);
    if (ret) {
		dev_err(&state->spi->dev, "READ_CHANNEL: Can't write seq config for channel %d", chan->channel);
		return ret;
	}

    ret = ad5592r_s_spi_noop(state, &rx);
	if (ret) {
		dev_err(&state->spi->dev, "READ_CHANNEL: SPI sync transfer failed on first noop");
		return ret;
	}

    ret = ad5592r_s_spi_noop(state, &rx);
	if (ret) {
		dev_err(&state->spi->dev, "READ_CHANNEL: SPI sync transfer failed on second noop");
		return ret;
	}

    u16 reg_val = get_unaligned_be16(&rx);
    *read_val = FIELD_GET(AD5592R_S_ADC_DATA_MASK, reg_val);
	//dev_info(&state->spi->dev, "READ_CHANNEL: SPI read rx %X", reg_val);
	//dev_info(&state->spi->dev, "READ_CHANNEL: SPI read read_val %d", *read_val);

    msg = 0;
    msg |= FIELD_PREP(AD5592R_S_WR_ADDR_MASK, AD5592R_S_ADC_SEQ_REG_ADDR);
	msg |= FIELD_PREP(AD5592R_S_WR_VALUE_MASK, AD5592R_S_STOP_CONV_CHANNELS);
	put_unaligned_be16(msg, &tx);
    //dev_info(&state->spi->dev, "READ_CHANNEL: write ADC_SEQ STOP tx: %X", tx);
    ret = spi_sync_transfer(state->spi, xfer, 1);
    if (ret) {
		dev_err(&state->spi->dev, "READ_CHANNEL: Can't write seq config for channel %d", chan->channel);
		return ret;
	}

    return 0;
}

static int ad5592r_s_read_raw(struct iio_dev *indio_dev,
			      struct iio_chan_spec const *chan, int *val_whole,
			      int *val_frac, long mask)
{
	struct ad5592r_s_state *state = iio_priv(indio_dev);
    int ret = 0;

	switch (mask) {
	case IIO_CHAN_INFO_ENABLE:
		*val_whole = state->enable;
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_RAW:
		if (state->enable) {
			ret = ad5592r_s_read_channel(indio_dev, chan, val_whole);
			if (ret) {
                dev_err(&state->spi->dev, "READ_ROW: SPI read failed on channel %d", chan->channel);
                return ret;
            }
			return IIO_VAL_INT;
		} else {
			return -EINVAL;
		}
	default:
		return -EINVAL;
	}
}

static int ad5592r_s_write_raw(struct iio_dev *indio_dev,
			       struct iio_chan_spec const *chan, int val_whole,
			       int val_frac, long mask)
{
	struct ad5592r_s_state *state = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_ENABLE:
		state->enable = val_whole;
		return 0;
	case IIO_CHAN_INFO_RAW:
		switch (chan->channel) {
		case 0:
			state->channel0 = val_whole;
			break;
		case 1:
			state->channel1 = val_whole;
			break;
		case 2:
			state->channel2 = val_whole;
			break;
		case 3:
			state->channel3 = val_whole;
			break;
		case 4:
			state->channel4 = val_whole;
			break;
		case 5:
			state->channel5 = val_whole;
			break;
		case 6:
			state->channel6 = val_whole;
			break;
		case 7:
			state->channel7 = val_whole;
			break;
		}
		return 0;
	default:
		return -EINVAL;
	}
}

static irqreturn_t ad5592r_s_trigger_handler(int irq, void *p)
{
    struct iio_poll_func *pf = p;
    struct iio_dev *indio_dev = pf->indio_dev;
    struct ad5592r_s_state *state = iio_priv(indio_dev);
    int buf[9];
    int i = 0;
    int bit = 0;
    int ret = 0;
    u16 read_data = 0;
    u16 tx = 0;
    u16 msg = 0;
    u16 data = 0;
    struct spi_transfer xfer[] = {
		{
			.tx_buf = &tx,
			.rx_buf = NULL,
			.len = 2,
		},
	};

    msg = 0;
    msg |= FIELD_PREP(AD5592R_S_WR_ADDR_MASK, AD5592R_S_ADC_SEQ_REG_ADDR);
	msg |= FIELD_PREP(AD5592R_S_WR_VALUE_MASK, (u8)*indio_dev->active_scan_mask);
    put_unaligned_be16(msg, &tx);
	//dev_info(&state->spi->dev, "TRIGG_HANDLE: write ADC_SEQ START tx: %X", tx);
    ret = spi_sync_transfer(state->spi, xfer, 1);
    if (ret) {
		dev_err(&state->spi->dev, "TRIGG_HANDLE: Can't write ADC_SEQ START");
		return IRQ_HANDLED;
	}

    ret = ad5592r_s_spi_noop(state, &read_data);
	if (ret) {
		dev_err(&state->spi->dev, "TRIGG_HANDLE: SPI transfer failed on first noop");
		return IRQ_HANDLED;
	}

    for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->num_channels) {
        ret = ad5592r_s_spi_noop(state, &read_data);
        if (ret) {
            dev_err(&state->spi->dev, "TRIGG_HANDLE: SPI transfer failed on noop for %d loop", bit);
            return IRQ_HANDLED;
        }

        data = get_unaligned_be16(&read_data);
        buf[i] = (int)FIELD_GET(AD5592R_S_ADC_DATA_MASK, data);
        i++;
    }

    msg = 0;
    msg |= FIELD_PREP(AD5592R_S_WR_ADDR_MASK, AD5592R_S_ADC_SEQ_REG_ADDR);
	msg |= FIELD_PREP(AD5592R_S_WR_VALUE_MASK, AD5592R_S_STOP_CONV_CHANNELS);
	put_unaligned_be16(msg, &tx);
    //dev_info(&state->spi->dev, "TRIGG_HANDLE: write ADC_SEQ STOP tx: %X", tx);
    ret = spi_sync_transfer(state->spi, xfer, 1);
    if (ret) {
		dev_err(&state->spi->dev, "TRIGG_HANDLE: Can't write ADC_SEQ STOP");
		return ret;
	}

    ret = iio_push_to_buffers(indio_dev, buf);
    if (ret) {
        dev_err(&state->spi->dev, "TRIGG_HANDLE: Failed PUSH to buffer");
        return IRQ_HANDLED;
    }

    iio_trigger_notify_done(indio_dev->trig);
    return IRQ_HANDLED;
}

static int ad5592r_s_debugfx(struct iio_dev *indio_dev, unsigned reg_addr,
			     unsigned write_val, unsigned *read_val)
{
	struct ad5592r_s_state *state = iio_priv(indio_dev);

	if (read_val) {
		return ad5592r_s_spi_read(state, reg_addr, (u16 *)read_val);
	} else {
		return ad5592r_s_spi_write(state, reg_addr, write_val);
	}
}

static int ad5592r_s_adc_init(struct iio_dev *indio_dev) {
    struct ad5592r_s_state *state = iio_priv(indio_dev);
    
    u16 tx = 0;
    u16 msg = 0;
    struct spi_transfer xfer[] = {
		{
			.tx_buf = &tx,
			.rx_buf = NULL,
			.len = 2,
		},
	};

    msg |= FIELD_PREP(AD5592R_S_WR_ADDR_MASK, AD5592R_S_ADC_CONFIG_REG_ADDR);
    msg |= FIELD_PREP(AD5592R_S_WR_VALUE_MASK, AD5592R_S_CONFIG_ADC_CHANNELS);
    put_unaligned_be16(msg, &tx);
    //dev_info(&state->spi->dev, "ADC_INIT: write ADC_CONFIG tx: %X", tx);
    int ret = spi_sync_transfer(state->spi, xfer, 1);
    if (ret) {
		dev_err(&state->spi->dev, "ADC_INIT: Can't write ADC_CONFIG");
		return ret;
	}

    msg = FIELD_PREP(AD5592R_S_WR_ADDR_MASK, AD5592R_S_VREF_REG_ADDR) | AD5592R_S_VREF_EN;
    put_unaligned_be16(msg, &tx);
    //dev_info(&state->spi->dev, "ADC_INIT: write ADC_VREF tx: %X", tx);
    ret = spi_sync_transfer(state->spi, xfer, 1);
    if (ret) {
		dev_err(&state->spi->dev, "ADC_INIT: Can't write ADC_VREF");
		return ret;
	}
    return 0;
}

static const struct iio_info ad5592r_s_info = {
	.read_raw = &ad5592r_s_read_raw,
	.write_raw = &ad5592r_s_write_raw,
	.debugfs_reg_access = &ad5592r_s_debugfx,
};

static int ad5592r_s_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	struct ad5592r_s_state *state;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*state));
	if (!indio_dev) {
		return -ENOMEM;
	}

	state = iio_priv(indio_dev);
	state->spi = spi;
	state->enable = 0;
	state->channel0 = 0;
	state->channel1 = 0;
	state->channel2 = 0;
	state->channel3 = 0;
	state->channel4 = 0;
	state->channel5 = 0;
	state->channel6 = 0;
	state->channel7 = 0;

	indio_dev->name = "ad5592r_s";
	indio_dev->info = &ad5592r_s_info;
	indio_dev->channels = ad5592r_s_channels;
	indio_dev->num_channels = ARRAY_SIZE(ad5592r_s_channels);

    if (ad5592r_s_adc_init(indio_dev)) {
        dev_err(&state->spi->dev, "Error on probe");
    }

    if(devm_iio_triggered_buffer_setup_ext(&spi->dev,
        indio_dev,
        NULL,
        ad5592r_s_trigger_handler,
        IIO_BUFFER_DIRECTION_IN,
        NULL,
        NULL))
    {
        dev_err(&state->spi->dev, "PROBE: Error on trigger_buffer_setup");
    }

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592r_s_driver =
{
    .driver = {
        .name = "ad5592r_s",
    },
    .probe = ad5592r_s_probe
};

module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Octavian Ionut CRACIUN");
MODULE_DESCRIPTION("AD5592R_S ADC Driver");
MODULE_LICENSE("GPL v2");
