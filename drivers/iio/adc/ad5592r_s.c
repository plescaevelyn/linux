// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>
#include <linux/bitfield.h>
#include <asm/unaligned.h>

#define AD5592R_S_REG_ADDR_MSK              GENMASK(14,11)
#define AD5592R_S_WR_VAL_MSK                GENMASK( 8, 0)
#define AD5592R_S_RDB_REG_SEL               GENMASK( 5, 2)
#define AD5592R_S_CHAN_CONFIG_MSK           GENMASK( 7, 0)
#define AD5592R_S_CHAN_DATA_CONV            GENMASK(11, 0)

#define AD5592R_S_RDB_EN                    BIT(6)
#define AD5592R_S_PD_EN                     BIT(9)

#define AD5592R_S_CHAN(x)                   BIT(x)

#define AD5592R_S_CONF_RDB_REG              0x7
#define AD5592R_S_PD_REF_CTRL               0xB
#define AD5592R_S_ADC_CONFIG                0x4
#define AD5592R_S_ADC_CHAN_INPUTS           0x3F
#define AD5592R_S_ADC_SEQ                   0x2

struct iio_ad5592r_s_state{
    bool en;
    int chan0; 
    int chan1;
    int chan2;
    int chan3;
    int chan4;
    int chan5;
    struct spi_device *spi;
};

struct iio_chan_spec const iio_ad5592r_s_chans[] = {
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

static int ad5592r_s_spi_nop(struct iio_ad5592r_s_state *st, u16 *val)
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

static int iio_ad5592r_s_read_chan(struct iio_ad5592r_s_state *st, struct iio_chan_spec const *chan, int *readval)
{    
    u16 rx = 0;
    u16 tx = 0;
    u16 msg = 0;
    u16 data = 0;
    int ret;

    struct spi_transfer xfer[] = {
        {
            .tx_buf = &tx,
            .rx_buf = NULL,
            .len = 2,
        }
    };

    msg |= FIELD_PREP(AD5592R_S_REG_ADDR_MSK, AD5592R_S_ADC_SEQ);
    msg |= FIELD_PREP(AD5592R_S_CHAN_CONFIG_MSK, AD5592R_S_CHAN(chan->channel));

    dev_info(&st->spi->dev, "READ CHAN : tx = 0x%X", msg);
    put_unaligned_be16(msg, &tx);
    dev_info(&st->spi->dev, "READ CHAN : msg = 0x%X", tx);

    ret = spi_sync_transfer(st->spi, xfer, 1);  
    if(ret){
        dev_err(&st->spi->dev, "Failed spi transfer config reg");
        return ret;
    }

    ret = ad5592r_s_spi_nop(st, &data);
    if(ret){
        dev_err(&st->spi->dev, "Failed nop transfer");
        return ret;
    }

    ret = ad5592r_s_spi_nop(st, &data);
    if(ret){
        dev_err(&st->spi->dev, "Failed nop transfer");
        return ret;
    }
    rx = get_unaligned_be16(&data);
    *readval = rx & AD5592R_S_CHAN_DATA_CONV;

    return 0;
}

static int iio_ad5592r_s_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val,
			int *val2,
			long mask)
{
    int ret;
    struct iio_ad5592r_s_state *st = iio_priv(indio_dev);
    switch (mask)
    {
    case IIO_CHAN_INFO_RAW:
        if(st->en){
            ret = iio_ad5592r_s_read_chan(st, chan, val);
            if(ret){
                dev_err(&st->spi->dev, "FAILED reading data from channels!");
                return ret;
            }
            return IIO_VAL_INT; 
        }else
            return -EINVAL;
    case IIO_CHAN_INFO_ENABLE:
        *val = st->en;
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
    case IIO_CHAN_INFO_RAW:
        if(st->en){
            switch (chan->channel){
                case 0: 
                    st->chan0 = val;
                    return 0;
                case 1: 
                    st->chan1 = val;
                    return 0;
                case 2:
                    st->chan2 = val;
                    return 0;
                case 3:
                    st->chan3 = val;
                    return 0;
                case 4:
                    st->chan4 = val;
                    return 0;
                case 5:
                    st->chan5 = val;
                    return 0;
                default:
                    return -EINVAL;
            }
        }else
            return -EINVAL;
    case IIO_CHAN_INFO_ENABLE:
        st->en = val;
        return 0;
    default:
        return -EINVAL;
    }

    return -EINVAL;
}


static int ad5592r_s_spi_readback(struct iio_ad5592r_s_state *st, u8 reg, u16 *readval) 
{
    u16 tx = 0;
    u16 msg = 0;
    u16 rx = 0;
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
    msg |= FIELD_PREP(AD5592R_S_REG_ADDR_MSK, AD5592R_S_CONF_RDB_REG);


    dev_info(&st->spi->dev, "tx = 0x%X", msg);
    put_unaligned_be16(msg, &tx);
    dev_info(&st->spi->dev, "msg = 0x%X", tx);

    ret = spi_sync_transfer(st->spi, xfer, 1);

    if(ret){
        dev_err(&st->spi->dev, "Failed spi transfer config reg");
        return ret;
    }

    ret = ad5592r_s_spi_nop(st, &rx);
    if(ret){
        dev_err(&st->spi->dev, "Failed nop transfer");
        return ret;
    }
    *readval = get_unaligned_be16(&rx);

    return 0;
}


static int ad5592r_s_spi_write(struct iio_ad5592r_s_state *st, u8 reg, u16 writeval) 
{
    u16 tx = 0;
    u16 msg = 0;
    struct spi_transfer xfer[] = {
        {
            .tx_buf = &msg,
            .rx_buf = NULL,
            .len = 2
        }
    };

    tx = FIELD_PREP(AD5592R_S_REG_ADDR_MSK, reg) | FIELD_PREP(AD5592R_S_WR_VAL_MSK, writeval);

    dev_info(&st->spi->dev, "tx = 0x%X", tx);
    put_unaligned_be16(tx, &msg);
    dev_info(&st->spi->dev, "msg = 0x%X", msg);

    return spi_sync_transfer(st->spi, xfer, 1);
}

static int ad5592r_s_enable_power_ref(struct iio_ad5592r_s_state *st){
    u16 msg = 0;
    u16 tx = 0;
    struct spi_transfer xfer[] = {
        {
            .rx_buf = NULL,
            .tx_buf = &tx,
            .len = 2,
        }
    };

    msg |= FIELD_PREP(AD5592R_S_REG_ADDR_MSK ,AD5592R_S_PD_REF_CTRL);
    msg |= AD5592R_S_PD_EN;

    dev_info(&st->spi->dev, "tx = 0x%X", msg);
    put_unaligned_be16(msg, &tx);
    dev_info(&st->spi->dev, "msg = 0x%X", tx); 

    return spi_sync_transfer(st->spi, xfer, 1);
}

static int ad5592r_s_adc_config(struct iio_ad5592r_s_state *st){
    u16 msg = 0;
    u16 tx = 0;
    struct spi_transfer xfer[] = {
        {
            .rx_buf = NULL,
            .tx_buf = &tx,
            .len = 2,
        }
    };

    msg |= FIELD_PREP(AD5592R_S_REG_ADDR_MSK, AD5592R_S_ADC_CONFIG);
    msg |= FIELD_PREP(AD5592R_S_CHAN_CONFIG_MSK, AD5592R_S_ADC_CHAN_INPUTS);

    dev_info(&st->spi->dev, "tx = 0x%X", msg);
    put_unaligned_be16(msg, &tx);
    dev_info(&st->spi->dev, "msg = 0x%X", tx); 

    return spi_sync_transfer(st->spi, xfer, 1);
}

static int ad5592r_s_debugfs(struct iio_dev *indio_dev,
				  unsigned reg, unsigned writeval,
				  unsigned *readval)
{
    struct iio_ad5592r_s_state *st = iio_priv(indio_dev);

    if(readval)
        return ad5592r_s_spi_readback(st, reg, (u16 *)readval);
    
    return ad5592r_s_spi_write(st, reg, writeval);
}

static const struct iio_info ad5592r_s_info = {
    .read_raw = &iio_ad5592r_s_read_raw,
    .write_raw = &iio_ad5592r_s_write_raw,
    .debugfs_reg_access = &ad5592r_s_debugfs,
};

static int ad5592r_s_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    struct iio_ad5592r_s_state *st;
    int ret;

    indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));

    if(!indio_dev)
        return -ENOMEM; //Error -> No memory
    indio_dev->name = "ad5592r_s";
    indio_dev->info = &ad5592r_s_info;
    indio_dev->channels = iio_ad5592r_s_chans;
    indio_dev->num_channels = ARRAY_SIZE(iio_ad5592r_s_chans);

    st = iio_priv(indio_dev);
    st->en = 0;
    st->chan0 = 0;
    st->chan1 = 0;
    st->chan2 = 0;
    st->chan3 = 0;
    st->chan4 = 0;
    st->chan5 = 0;
    st->spi = spi;

    ret = ad5592r_s_enable_power_ref(st);
    if(ret){
        dev_err(&spi->dev, "FAILED disable external power reference!");
    }

    ret = ad5592r_s_adc_config(st);
    if(ret){
        dev_err(&spi->dev, "FAILED configuring ADC inputs!");
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

MODULE_AUTHOR("Zaicanu");
MODULE_DESCRIPTION("Analog Devices AD5592R_S");
MODULE_LICENSE("GPL v2");