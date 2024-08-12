// SPDX-License-Identifier: GPL-2.0-or-later
/*
* Copyright (C) 2024 Analog Devices, Inc.
*/

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>
#include <linux/bitfield.h>
#include <asm/unaligned.h>


#define AD5592R_S_ADDR_WR_MSK       GENMASK(14, 11)
#define AD5592R_S_VAL_WR_MSK        GENMASK(8, 0)

#define AD5592R_S_READBACK_REG_SEL  GENMASK(5, 2)
#define AD5592R_S_RDB_ENABLE        BIT(6)

#define AD5592R_S_CONF_RDB_REG      0x7

struct ad5592r_s_state {
    struct spi_device *spi;
    bool en;
    int chan0;
    int chan1;
    int chan2;
    int chan3;
    int chan4;
    int chan5;
};

struct iio_chan_spec const iio_ad5592r_chans[] = {
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
    },

   

};


static int iio_ad5592r_read_raw (struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val,
			int *val2,
			long mask)
{
    struct ad5592r_s_state *st = iio_priv(indio_dev);

    switch(mask)
    {
    case IIO_CHAN_INFO_RAW:
        if (st->en){
            switch (chan->channel)
            {
            case 0:
                *val = st->chan0;
                break;
            case 1:
                *val = st->chan1;
                break;
            case 2:
                *val = st->chan2;
                break;
            case 3:
                *val = st->chan3;
                break;
            case 4:
                *val = st->chan4;
                break;
            case 5:
                *val = st->chan5;
                break;
            default:
                return -EINVAL;
                break;
            }
            return IIO_VAL_INT;
        }
        else
            return -EINVAL;
    case IIO_CHAN_INFO_ENABLE:
        *val = st->en;
        return IIO_VAL_INT;

    default:
        return -EINVAL;
    }

    return -EINVAL;

}

static int iio_ad5592r_write_raw (struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int val,
			int val2,
			long mask)
{
    struct ad5592r_s_state *st = iio_priv(indio_dev);

    switch(mask)
    {
    case IIO_CHAN_INFO_ENABLE:
        st->en = val;
        return 0;
    case IIO_CHAN_INFO_RAW:
        if (st->en){
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
            default:
                return -EINVAL;
                break;
            }
            return 0;
        }
        else
            return -EINVAL;

    default:
        return -EINVAL;
    }

    return -EINVAL;

}


static int ad5592r_s_spi_write (struct ad5592r_s_state *st, u8 reg, u16 writeval)
{
    u16 tx = 0;
    u16 msg = 0;
    int ret;


    struct spi_transfer xfer[] =
    {
        {
        .tx_buf = &msg,
        .rx_buf = NULL,
        .len = 2,
        }
    };

    tx = FIELD_PREP(AD5592R_S_ADDR_WR_MSK, reg) | 
         FIELD_PREP(AD5592R_S_VAL_WR_MSK, writeval);

    dev_info(&st->spi->dev, "ad5592r_s_spi_write: tx = %X\n", tx);

    put_unaligned_be16(tx, &msg);

    dev_info(&st->spi->dev, "ad5592r_s_spi_write: msg = %X\n", msg);


    ret = spi_sync_transfer(st->spi, xfer, 1);
    if (ret)
    {
        dev_err(&st->spi->dev, "SPI sync transfer fail during write");
        return ret;
    }

    return 0;
}

static int ad5592r_s_spi_nop(struct ad5592r_s_state *st, u16 *val)
{
    u16 tx = 0; 

    struct spi_transfer xfer [] = {
        {
            .tx_buf = &tx,
            .rx_buf = val,
            .len = 2,
        }
    };

    return spi_sync_transfer(st->spi, xfer, 1);
}


static int ad5592r_s_spi_read(struct ad5592r_s_state *st, u8 reg, u16 *readval)
{
    u16 tx = 0;
    u16 rx = 0;
    u16 msg = 0;
    int ret;

    struct spi_transfer xfer[] = {
        {
            .tx_buf = &msg,
            .rx_buf = NULL,
            .len = 2,
        }
    };
    
    tx |= FIELD_PREP(AD5592R_S_ADDR_WR_MSK, AD5592R_S_CONF_RDB_REG);
    tx |= AD5592R_S_RDB_ENABLE;
    tx |= FIELD_PREP(AD5592R_S_READBACK_REG_SEL, reg);

    dev_info(&st->spi->dev, "ad5592r_s_spi_read: tx = 0x%X\n", tx);

    put_unaligned_be16(tx, &msg);

    dev_info(&st->spi->dev, "ad5592r_s_spi_read: msg = 0x%X\n", msg);

    ret = spi_sync_transfer(st->spi, xfer, 1);
    if (ret)
    {
        dev_err(&st->spi->dev, "Failed spi transfer config reg");
        return ret;
    }

    ret = ad5592r_s_spi_nop(st, &rx);
    if (ret)
    {
        dev_err(&st->spi->dev, "Failed nop transfer");
        return ret;
    }

    *readval = get_unaligned_be16(&rx);

    return 0;
}

static int ad5592r_s_debugfs(struct iio_dev *indio_dev,
                             unsigned reg, 
                             unsigned writeval,
                             unsigned *readval)
{
    struct ad5592r_s_state *st = iio_priv(indio_dev);

    if (readval)
    {
        return ad5592r_s_spi_read(st, reg, (u16 *) readval);
    }
    else return ad5592r_s_spi_write(st, reg, writeval);

}

static const struct iio_info ad5592r_s_info = {
    .read_raw = &iio_ad5592r_read_raw,
    .write_raw = &iio_ad5592r_write_raw,
    .debugfs_reg_access = &ad5592r_s_debugfs,


};


static int ad5592r_s_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    //int ret;

    struct ad5592r_s_state *st;

    indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
    if (!indio_dev)
        return -ENOMEM;

    indio_dev->name = "ad5592r_s";
    indio_dev->info = &ad5592r_s_info;
    indio_dev->channels = iio_ad5592r_chans;
    indio_dev->num_channels = ARRAY_SIZE(iio_ad5592r_chans);


    st = iio_priv(indio_dev);
    st->en = 0;
    st->chan0 = 0;
    st->chan1 = 0;
    st->chan2 = 0;
    st->chan3 = 0;
    st->chan4 = 0;
    st->chan5 = 0;
    st->spi = spi;


    return devm_iio_device_register(&spi->dev, indio_dev);

}

static struct spi_driver ad5592r_s_driver = {
    .driver = {
        .name = "ad5592r_s",
    },
    .probe = ad5592r_s_probe, 
};
module_spi_driver(ad5592r_s_driver);


MODULE_AUTHOR("Nechita Florina <nechitaflorina2002@gmail.com>");
MODULE_DESCRIPTION("Analog Devices AD5592R");
MODULE_LICENSE("GPL v2");