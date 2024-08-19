// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <linux/spi/spi.h>
#include <linux/module.h>

#include <linux/iio/iio.h>

#include <linux/bitfield.h>
#include <asm/unaligned.h>

#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/buffer.h>

#define AD5592R_S_WR_ADDR_MSK               GENMASK(14, 11)
#define AD5592R_S_WR_DATA_MSK               GENMASK(9, 0)

#define AD5592R_S_RDB_EN                    BIT(6)
#define AD5592R_S_RDB_REG_SEL               GENMASK(5, 2)
#define AD5592R_S_CONF_RDB_REG              0x7

#define AD5592R_S_ADC_SEQ_REG               0x2
#define AD5592R_S_AD_CHANNEL(y)             BIT(y)

#define AD5592R_S_ADC_CONF_REG              0x4
#define AD5592R_S_ADC_CONF_PINS             GENMASK(5, 0)

#define AD5592R_S_PD_REF_CTRL_REG           0xB
#define AD5592R_S_PD_REF_CTRL_EN_REF        BIT(9)

struct ad5592r_state {
    struct spi_device* spi;
    bool en;
    int chan[6];
};

static int ad5592r_s_spi_noop(struct ad5592r_state* st,
                              u16* val)
{
    u16 tx = 0;
    struct spi_transfer xfer = {
        .tx_buf = &tx,
        .rx_buf = val,
        .len = 2,
    };

    return spi_sync_transfer(st->spi, &xfer, 1);
}

static int ad5592r_s_spi_read(struct ad5592r_state* st,
                              u8 reg,
                              u16* readval)
{
    u16 tx = 0;
    u16 msg;
    u16 rx = 0;
    int ret;
    struct spi_transfer xfer = {
        .rx_buf = NULL,
        .tx_buf = &msg,
        .len = 2,
    };

    tx = AD5592R_S_RDB_EN | FIELD_PREP(AD5592R_S_RDB_REG_SEL, reg) | FIELD_PREP(AD5592R_S_WR_ADDR_MSK, AD5592R_S_CONF_RDB_REG);

    //dev_info(&st->spi->dev, "tx = 0x%X", tx);
    put_unaligned_be16(tx, &msg);
    //dev_info(&st->spi->dev, "msg = 0x%X", msg);

    ret = spi_sync_transfer(st->spi, &xfer, 1);
    if (ret)
    {
        dev_err(&st->spi->dev, "SPI sync transfer fail during config part of read");
        return ret;
    }

    ret = ad5592r_s_spi_noop(st, &rx);
    if (ret)
    {
        dev_err(&st->spi->dev, "SPI sync transfer fail during noop part of read");
        return ret;
    }

    *readval = get_unaligned_be16(&rx);

    return 0;
}

static int ad5592r_s_spi_write(struct ad5592r_state* st,
                               u8 reg,
                               u16 writeval)
{
    u16 tx = 0;
    u16 msg;
    int ret;
    struct spi_transfer xfer = {
        .tx_buf = &msg,
        .rx_buf = NULL,
        .len = 2,
    };

    tx = FIELD_PREP(AD5592R_S_WR_ADDR_MSK, reg) | FIELD_PREP(AD5592R_S_WR_DATA_MSK, writeval);

    //dev_info(&st->spi->dev, "tx = 0x%X", tx);
    put_unaligned_be16(tx, &msg);
    //dev_info(&st->spi->dev, "msg = 0x%X", msg);

    ret = spi_sync_transfer(st->spi, &xfer, 1);
    if (ret)
    {
        dev_err(&st->spi->dev, "SPI sync transfer fail during write");
        return ret;
    }

    return 0;
}

static int ad5592r_s_chan_read(struct ad5592r_state* st,
                               struct iio_chan_spec* chan,
                               u16* readval)
{
    int ret;

    ret = ad5592r_s_spi_write(st, AD5592R_S_ADC_SEQ_REG, AD5592R_S_AD_CHANNEL(chan->channel));
    if (ret)
    {
        dev_err(&st->spi->dev, "failed during conversion reg write");
        return ret;
    }

    u16 rx;

    ret = ad5592r_s_spi_noop(st, &rx);
    if (ret)
    {
        dev_err(&st->spi->dev, "failed during noop");
        return ret;
    }

    ret = ad5592r_s_spi_noop(st, &rx);
    if (ret)
    {
        dev_err(&st->spi->dev, "failed during (noop) read");
        return ret;
    }

    u16 temp = get_unaligned_be16(&rx);

    int adc_address = 0;
    adc_address = (temp & 0x7000) >> 12;
    *readval = temp & 0x0FFF;

    //dev_info(&st->spi->dev, "adc address: %d", adc_address);
    //dev_info(&st->spi->dev, "adc value: %d", *readval);
    
    return 0;
}

static int ad5592r_read_raw(struct iio_dev* indio_dev, 
                            struct iio_chan_spec const *chan, 
                            int* val,
                            int* val2,
                            long mask)
{
    int ret;
    struct ad5592r_state *st = iio_priv(indio_dev);

    switch (mask)
    {
        case IIO_CHAN_INFO_ENABLE:
        {
            *val = st->en;

            return IIO_VAL_INT;
        }
        case IIO_CHAN_INFO_RAW:
        {
            if (st->en == 1)
            {
                ret = ad5592r_s_chan_read(st, chan, (u16*)val);
                if (ret)
                {
                    dev_err(&st->spi->dev, "failed during channel read");
                    return ret;
                }
            }
            else
            {
                return -EINVAL;
            }

            return IIO_VAL_INT;
        }
        default:
        {
            return -EINVAL;
        }
    }

    return -EINVAL;
}

static int ad5592r_write_raw(struct iio_dev* indio_dev,
                             struct iio_chan_spec const *chan,
                             int val,
                             int val2,
                             long mask)
{
    struct ad5592r_state *st = iio_priv(indio_dev);

    switch (mask)
    {
        case IIO_CHAN_INFO_ENABLE:
        {
            st->en = (bool)val;

            return 0;
        }
        case IIO_CHAN_INFO_RAW:
        {
            if (st->en)
            {
                st->chan[chan->channel] = val;

                return 0;
            }
            else
            {
                return -EINVAL;
            }
        }
        default:
        {
            return -EINVAL;
        }
    }

    return -EINVAL;
}

static irqreturn_t ad5592r_s_trigger_handler(int irq, void *p)
{
    struct iio_poll_func* pf = p;
    struct iio_dev* indio_dev = pf->indio_dev;
    struct ad5592r_state* st = iio_priv(indio_dev);

    u16 buf[6];
    int bit = 0;
    int ret;

    int i = 0;

    for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->num_channels)
    {
        ad5592r_s_chan_read(st, &indio_dev->channels[bit], &buf[i]);
        ++i;
    }

    ret = iio_push_to_buffers(indio_dev, buf);
    if (ret)
    {
        dev_err(&st->spi->dev, "failed during pushing buffer to device");
        return IRQ_HANDLED;
    }

    iio_trigger_notify_done(indio_dev->trig);

    return IRQ_HANDLED;
}

static int ad5592r_s_debugfs(struct iio_dev* indio_dev,
                             unsigned reg,
                             unsigned writeval,
                             unsigned* readval)
{
    struct ad5592r_state* st = iio_priv(indio_dev);

    if (readval)
    {
        return ad5592r_s_spi_read(st, reg, (u16*)readval);
    }

    return ad5592r_s_spi_write(st, reg, writeval);
}

static const struct iio_info ad5592r_info = {
    .read_raw = &ad5592r_read_raw,
    .write_raw = &ad5592r_write_raw,
    .debugfs_reg_access = &ad5592r_s_debugfs,
};

static const struct iio_chan_spec ad5592r_channels[] = {
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
        },
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
        },
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
        },
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
        },
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
        },
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
        },
    }
};

static int ad5592r_s_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    struct ad5592r_state *st;
    int ret;

    indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
    if (!indio_dev)
    {
        return -ENOMEM;
    }

    indio_dev->name = "ad5592r_s";
    indio_dev->info = &ad5592r_info;
    indio_dev->channels = ad5592r_channels;
    indio_dev->num_channels = ARRAY_SIZE(ad5592r_channels);

    st = iio_priv(indio_dev);
    st->en = 0;
    for (int i = 0; i < ARRAY_SIZE(ad5592r_channels); ++i)
    {
        st->chan[i] = 0;
    }
    st->spi = spi;

    ret = ad5592r_s_spi_write(st, AD5592R_S_PD_REF_CTRL_REG, AD5592R_S_PD_REF_CTRL_EN_REF);
    if (ret)
    {
        dev_err(&st->spi->dev, "failed during internal voltage reference config");
        return ret;
    }

    ret = ad5592r_s_spi_write(st, AD5592R_S_ADC_CONF_REG, AD5592R_S_ADC_CONF_PINS);
    if (ret)
    {
        dev_err(&st->spi->dev, "failed during adc pins config");
        return ret;
    }

    ret = devm_iio_triggered_buffer_setup_ext(&spi->dev, indio_dev, NULL, ad5592r_s_trigger_handler, IIO_BUFFER_DIRECTION_IN, NULL, NULL);
    if (ret)
    {
        dev_err(&spi->dev, "failed during buffer setup");
        return ret;
    }

    return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592r_s_driver = {
    .driver = {
        .name = "ad5592r_s",
    },
    .probe = ad5592r_s_probe,
};

module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Cristea Tudor");
MODULE_DESCRIPTION("Analog Devices ADC AD5592R");
MODULE_LICENSE("GPL v2");