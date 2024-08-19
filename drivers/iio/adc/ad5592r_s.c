// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <linux/bitfield.h>
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>
#include <asm/unaligned.h>
#include <linux/iio/buffer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>

#define AD5592R_S_WR_ADDR_MSK           GENMASK(14, 11)
#define AD5592R_S_WR_VAL_MSK            GENMASK(9, 0)
#define AD5592R_S_RDB_REG_SEL           GENMASK(5, 2)
#define AD5592R_S_RDB_EN                BIT(6)
#define AD5592R_S_CONF_RDB_REG          0x7

#define AD5592R_S_REG_PD_REF            0b1011
#define AD5592R_S_REG_PD_REF_INT        BIT(9)
#define AD5592R_S_REG_CONFIG_ADC        0b0100
#define AD5592R_S_REG_CHAN_SEL          GENMASK(5,0)
#define AD5592R_S_REG_SEQ               0b0010

struct iio_ad5592r_s_state {
    struct spi_device *spi;
    bool en;
    int chan0;
    int chan1;
    int chan2;
    int chan3;
    int chan4;
    int chan5;
    int chan6;
    int chan7;    
};

struct iio_chan_spec const iio_ad5592r_s_chans[] = {
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
    },
};

static int ad5592r_s_read_chan(struct iio_ad5592r_s_state *st,
			                    const struct iio_chan_spec *chan,
                                int *val);
static int iio_ad5592r_s_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val,
			int *val2,
			long mask)
{
    struct iio_ad5592r_s_state *st = iio_priv(indio_dev);
    switch (mask)
    {
        case IIO_CHAN_INFO_RAW:
            if(st->en)
            {
                ad5592r_s_read_chan(st, chan, val);
                return IIO_VAL_INT; 
            }
            else
                return -EINVAL;
        case IIO_CHAN_INFO_ENABLE:
            *val=st->en;
             return IIO_VAL_INT;
        return IIO_VAL_INT;    
        default:
            return -EINVAL;
    }
    return -EINVAL;
}

static int iio_ad5592r_s_write_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int val,
			int val2,
			long mask)
{
    struct iio_ad5592r_s_state *st = iio_priv(indio_dev);
    switch (mask)
    {
        case IIO_CHAN_INFO_ENABLE:
            st->en = val;
            return 0;
        case IIO_CHAN_INFO_RAW:
            if(st->en)
            {
                switch (chan->channel)
                {
                    case 0:
                       st->chan0 = val;
                        break;
                    case 1:
                       st->chan1 = val;
                        break;
                    case 2:
                        st->chan2 = val;
                        break;
                    case 3:
                        st->chan3 = val;
                        break;
                    case 4:
                        st->chan4 = val;
                        break;
                    case 5:
                        st->chan5 = val;
                        break;
                    case 6:
                        st->chan6 = val;
                        break;
                    case 7:
                        st->chan7 = val;
                        break;
                }
                return 0;    
            }
            else
                return -EINVAL;
        default:
            return -EINVAL;
    }
}

static int ad5592r_s_spi_write(struct iio_ad5592r_s_state *st,
                                u8 reg,
                                u16 writeval)
{
    u16 tx = 0;
    u16 msg = 0;
    struct spi_transfer xfer[] = {
        {
            .tx_buf = &tx,
            .rx_buf = NULL,
            .len = 2
        }
    };

    msg = FIELD_PREP(AD5592R_S_WR_ADDR_MSK, reg) | FIELD_PREP(AD5592R_S_WR_VAL_MSK, writeval);
    //dev_info(&st->spi->dev, "tx = 0x%X", tx);
    put_unaligned_be16(msg, &tx);
    //dev_info(&st->spi->dev, "msg = 0x%X", msg);
    
    return spi_sync_transfer(st->spi, xfer, 1);
};

static int ad5592r_s_spi_nop (struct iio_ad5592r_s_state *st, u16 *val)
{
    u16 tx = 0;
    struct spi_transfer xfer[] = {
        {
            .tx_buf = &tx,
            .rx_buf = val,
            .len = 2
        }
    };

    return spi_sync_transfer(st->spi, xfer, 1);
}

static int ad5592r_s_spi_read(struct iio_ad5592r_s_state *st,
                                u8 reg,
                                u16 *readval)
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

    tx |= FIELD_PREP(AD5592R_S_WR_ADDR_MSK, AD5592R_S_CONF_RDB_REG);
    tx |= AD5592R_S_RDB_EN;
    tx |= FIELD_PREP(AD5592R_S_RDB_REG_SEL, reg);
    
    //dev_info(&st->spi->dev, "tx = 0x%X", tx);
    //dev_info(&st->spi->dev, "msg = 0x%X", msg);
    
    put_unaligned_be16(tx, &msg);
    ret = spi_sync_transfer(st->spi, xfer, 1);
    if (ret)
    {
        dev_err(&st->spi->dev, "Failed spi transfer config reg");
        return ret;
    }

    ret = ad5592r_s_spi_nop(st, &rx);
    if (ret)
    {
        dev_err(&st->spi->dev, "Failed spi nop transfer");
        return ret;
    }
    *readval = get_unaligned_be16(&rx);

    return 0;
    
}

static int ad5592r_s_read_chan(struct iio_ad5592r_s_state *st,
			                    const struct iio_chan_spec *chan,
                                int *val)
{
    u16 rx = 0;
    int ret;

    ret = ad5592r_s_spi_write(st, AD5592R_S_REG_SEQ, BIT(chan->channel));
    if (ret)
    {
        dev_err(&st->spi->dev, "Failed spi select channel");
        return ret;
    }

    ret = ad5592r_s_spi_nop(st, &rx);
    if (ret)
    {
        dev_err(&st->spi->dev, "Failed spi first nop");
        return ret;
    }

    ret = ad5592r_s_spi_nop(st, &rx);
    if (ret)
    {
        dev_err(&st->spi->dev, "Failed spi second nop");
        return ret;
    }

    //dev_info(&st->spi->dev, "tx = 0x%X", rx);
    *val = get_unaligned_be16(&rx);
    *val = *val & GENMASK(11,0);
    return 0;
}

static irqreturn_t iio_ad5592r_s_trig_handler(int irq, void *p)
{
    struct iio_poll_func *pf = p;
    struct iio_dev *indio_dev = pf->indio_dev;
    struct iio_ad5592r_s_state *st = iio_priv(indio_dev);\
    const struct iio_chan_spec *chan = indio_dev->channels;

    u16 buf[6];
    int val;
    int bit, ret, i = 0;

    for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->num_channels)
    {
        ad5592r_s_read_chan(st, &chan[bit], &val);
        if(ret)
        {
            dev_err(&st->spi->dev, "Error read channel %d", bit);
            return ret;
        }
        buf[i++]=val;
    }    
    ret = iio_push_to_buffers(indio_dev, buf);
    if(ret)
    {
        dev_err(&st->spi->dev, "FAILED push to buffers");
        return IRQ_HANDLED;
    }
    iio_trigger_notify_done(indio_dev->trig);
    return IRQ_HANDLED;
}

static int ad5592r_s_debugfs(struct iio_dev *indio_dev,
			unsigned int reg, unsigned int writeval,
			unsigned int *readval)
{
    struct iio_ad5592r_s_state *st = iio_priv(indio_dev);
    if(readval)
        return ad5592r_s_spi_read(st, reg, (u16 *) readval);
    return ad5592r_s_spi_write(st, reg, (u16) writeval);
} 


static const struct iio_info iio_ad5592r_s_info = {
    .read_raw = &iio_ad5592r_s_read_raw,
    .write_raw = &iio_ad5592r_s_write_raw,
    .debugfs_reg_access = &ad5592r_s_debugfs,
}; 

static int iio_ad5592r_s_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    struct iio_ad5592r_s_state *st;
    int ret;

    indio_dev = devm_iio_device_alloc(&spi->dev, 0);
    if(!indio_dev)
        return ENOMEM;

    indio_dev->name = "iio-ad5592r_s";
    indio_dev->info = &iio_ad5592r_s_info;
    indio_dev->channels = iio_ad5592r_s_chans;
    indio_dev->num_channels = ARRAY_SIZE(iio_ad5592r_s_chans);

    st = iio_priv(indio_dev);
    st->spi=spi;
    st->en = 1;
    st->chan0 = 0;
    st->chan1 = 0;
    st->chan2 = 0;
    st->chan3 = 0;
    st->chan4 = 0;
    st->chan5 = 0;
    st->chan6 = 0;
    st->chan7 = 0;

    ret = devm_iio_triggered_buffer_setup_ext(&spi->dev, indio_dev, NULL,
                                            iio_ad5592r_s_trig_handler, IIO_BUFFER_DIRECTION_IN, NULL, NULL);

    ret = ad5592r_s_spi_write(st, AD5592R_S_REG_PD_REF, AD5592R_S_REG_PD_REF_INT);
    if(ret)
    {
        dev_err(&st->spi->dev, "FAILED setting internal reference");
        return ret;
    }
    ret = ad5592r_s_spi_write(st, AD5592R_S_REG_CONFIG_ADC, AD5592R_S_REG_CHAN_SEL);
    if(ret)
    {
        dev_err(&st->spi->dev, "FAILED setting channels for ADC conversion");
        return ret;
    }
    return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver iio_ad5592r_s_driver = {
    .driver = {
        .name = "iio-ad5592r_s"
    },
    .probe = iio_ad5592r_s_probe
};

module_spi_driver(iio_ad5592r_s_driver);

MODULE_AUTHOR("Ghisa Alexandru");
MODULE_DESCRIPTION("Analog Devices IIO ADC");
MODULE_LICENSE("GPL v2");