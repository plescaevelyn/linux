#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>
#include <linux/bitfield.h>
#include <asm/unaligned.h>

#define AD5592R_S_WR_ADDR_MASK                      GENMASK(14,11)
#define AD5592R_S_WR_VALUE_MASK                     GENMASK(8,0)

#define AD5592R_S_RDBK_REG_SEL_MASK                 GENMASK(5,2)
#define AD5592R_S_RDBK_EN_MASK                      BIT(6)

#define AD5592R_S_CONF_RDBK_REG_ADDR                0x7


struct ad5582r_s_state {
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
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 1,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 2,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 3,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 4,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 5,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 6,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 7,
    }
};

static int ad5592r_s_read_raw(struct iio_dev* indio_dev,
    struct iio_chan_spec const *chan,
    int *val_whole,
    int *val_frac,
    long mask) {
        struct ad5582r_s_state *state = iio_priv(indio_dev);

        switch (mask) {
            case IIO_CHAN_INFO_ENABLE:
                *val_whole = state->enable;
                return IIO_VAL_INT;
            case IIO_CHAN_INFO_RAW:
                if (state->enable) {
                    switch (chan->channel) {
                        case 0:
                            *val_whole = state->channel0;
                            break;
                        case 1:
                            *val_whole = state->channel1;
                            break;
                        case 2:
                            *val_whole = state->channel2;
                            break;
                        case 3:
                            *val_whole = state->channel3;
                            break;
                        case 4:
                            *val_whole = state->channel4;
                            break;
                        case 5:
                            *val_whole = state->channel5;
                            break;
                        case 6:
                            *val_whole = state->channel6;
                            break;
                        case 7:
                            *val_whole = state->channel7;
                            break;     
                    }
                    return IIO_VAL_INT;
                } else {
                    return -EINVAL;
                }
            default:
                return -EINVAL;
        }
}

static int ad5592r_s_write_raw(struct iio_dev* indio_dev,
    struct iio_chan_spec const *chan,
    int val_whole,
    int val_frac,
    long mask) {
        struct ad5582r_s_state *state = iio_priv(indio_dev);

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


static int ad5592r_s_spi_noop(struct ad5582r_s_state *state, u16 *ret_val) {
    u16 tx = 0;
    struct  spi_transfer xfer[] = {
        {
            .rx_buf = ret_val,
            .tx_buf = &tx,
            .len = 2,
        }
    };
    return spi_sync_transfer(state->spi, xfer, 1);
} 

static int ad5592r_s_spi_read(struct ad5582r_s_state *state,
    u8 reg_addr,
    u16 *read_val) {
        u16 tx = 0;
        u16 rx = 0;
        u16 msg = 0;
        struct  spi_transfer xfer[] = {
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
        dev_info(&state->spi->dev, "SPI read msg: %X", msg);
        dev_info(&state->spi->dev, "SPI read tx %X", tx);

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
        dev_info(&state->spi->dev, "SPI read rx %X", rx);
        dev_info(&state->spi->dev, "SPI read read_val %X", *read_val);

        return 0;
}

static int ad5592r_s_spi_write(struct ad5582r_s_state *state,
    u8 reg_addr,
    u16 write_val) {
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
        dev_info(&state->spi->dev, "SPI write msg: %X", msg);
        dev_info(&state->spi->dev, "SPI write tx: %X", tx);

        int ret = spi_sync_transfer(state->spi, xfer, 1);
        if (ret) {
            dev_err(&state->spi->dev, "SPI sync transfer failed on write");
            return ret;
        }

        return 0;
    }

static int ad5592r_s_debugfx(struct iio_dev *indio_dev,
    unsigned reg_addr,
    unsigned write_val,
    unsigned *read_val) {
        struct ad5582r_s_state *state = iio_priv(indio_dev);

        if (read_val) {
            return ad5592r_s_spi_read(state, reg_addr, (u16 *)read_val);
        } else {
            return ad5592r_s_spi_write(state, reg_addr, write_val);
        }
}

static const struct iio_info ad5592r_s_info = {
    .read_raw = &ad5592r_s_read_raw,
    .write_raw = &ad5592r_s_write_raw,
    .debugfs_reg_access = &ad5592r_s_debugfx,
};

static int ad5592r_s_probe(struct spi_device *spi) {
    struct iio_dev *indio_dev;
    struct ad5582r_s_state *state;

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
