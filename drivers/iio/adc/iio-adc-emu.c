// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 Analog Devices
 */

#include<asm/unaligned.h>
#include<linux/bitfield.h>
#include <linux/spi/spi.h>
#include <linux/module.h>

#include <linux/iio/buffer.h>
#include <linux/iio/iio.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h> 

#define IIO_ADC_EMU_READ_MSK            BIT(7)
#define IIO_ADC_ADDR_RD_MSK             GENMASK(6, 0)
#define IIO_ADC_ADDR_WR_MSK             GENMASK(14, 8)
#define IIO_ADC_VAL_MSK                 GENMASK(7, 0)

#define IIO_ADC_EMU_REG_CNVST           0x3
#define IIO_ADC_EMU_REG_POWERON         0x2  
#define IIO_ADC_EMU_REG_CHAN_HIGH(x)    0x4 + x * 2
#define IIO_ADC_EMU_REG_CHAN_LOW(x)     0X5 + x * 2
#define IIO_ADC_EMU_REG_CHIP_ID         0x0
#define IIO_ADC_EMU_CNVST_EN            BIT(0)

struct iio_adc_emu_state
{
    struct spi_device *spi;
    bool en;
    int chan0;
    int chan1;
};

struct iio_chan_spec const iio_adc_emu_chans[] = {
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
    }
};

static int iio_adc_emu_spi_read(struct iio_adc_emu_state *st,
                                u8 reg, 
                                u8 *readval)
{
    u8 tx;
    u8 rx;
    int ret;
    struct spi_transfer xfer[] = {
        {
            .rx_buf = NULL,
            .tx_buf = &tx,
            .len = 1,
        },
        {
            .rx_buf = &rx,
            .tx_buf = NULL,
            .len = 1,
        }
    };

    tx = IIO_ADC_EMU_READ_MSK | FIELD_PREP(IIO_ADC_ADDR_RD_MSK, reg);

    ret = spi_sync_transfer(st->spi, xfer, 2);
    if(ret)
    {
        dev_err(&st->spi->dev, "Spi sync transfer fail during read");
        return ret;
    }

    *readval = rx;
    return 0;
}

static int iio_adc_emu_spi_write(struct iio_adc_emu_state *st,
                                u8 reg, 
                                u8 writeval)
{
    u16 tx = 0;
    u16 msg;
    struct spi_transfer xfer = {
        .rx_buf = NULL,
        .tx_buf = &msg,
        .len = 2,
    };

    tx = FIELD_PREP(IIO_ADC_ADDR_WR_MSK, reg) | 
         FIELD_PREP(IIO_ADC_VAL_MSK, writeval);

    dev_info(&st->spi->dev, "tx = 0x%X", tx);

    put_unaligned_be16(tx, &msg);
    dev_info(&st->spi->dev, "msg = 0x%X", msg);
    
    return spi_sync_transfer(st->spi, &xfer, 1);
}

static int iio_adc_emu_read_chan(struct iio_adc_emu_state *st, struct iio_chan_spec *chan, int *val)
{
    int ret;
    u8 high;
    u8 low;

    ret = iio_adc_emu_spi_write(st, IIO_ADC_EMU_REG_CNVST, IIO_ADC_EMU_CNVST_EN);
    if(ret) {
        dev_err(&st->spi->dev, "FAILED conversion reg write");
        return ret;
    }


    ret = iio_adc_emu_spi_read(st, IIO_ADC_EMU_REG_CHAN_HIGH(chan->channel), &high);
    if(ret) {
        dev_err(&st->spi->dev, "FAILED read chan high");
        return ret;
    }

    ret = iio_adc_emu_spi_read(st, IIO_ADC_EMU_REG_CHAN_LOW(chan->channel), &low);
    if(ret) {
        dev_err(&st->spi->dev, "FAILED read chan low");
        return ret;
    }

    *val = (high << 8) | low;

    return 0;
}

static int iio_adc_emu_write_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int val,
			int val2,
			long mask)
{
    struct iio_adc_emu_state *st = iio_priv(indio_dev);

    switch (mask)
    {
    case IIO_CHAN_INFO_ENABLE:
        st->en = val;
        return 0;
    default: 
        return -EINVAL;
    }
    return -EINVAL;
}

static int iio_adc_emu_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val,
			int *val2,
			long mask)
{
    struct iio_adc_emu_state *st = iio_priv(indio_dev);\
    int ret;

    switch (mask)
    {
    case IIO_CHAN_INFO_RAW:
        if(st->en)
        {
            iio_adc_emu_read_chan(st, chan, val);
            if(ret) {
                dev_err(&st->spi->dev, "Error reading from channel");
            }
            return 0;
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

static irqreturn_t  iio_adc_emu_trig_handle(int iqr, void *p)
{
    struct iio_poll_func *pf = p;
    struct iio_dev *indio_dev = pf->indio_dev;
    struct iio_adc_emu_state *st = iio_priv(indio_dev);
    u16 buf[2];
    int bit = 0;
    int ret;
    int i = 0;
    u8 high;
    u8 low;

    ret = iio_adc_emu_spi_write(st, IIO_ADC_EMU_REG_CNVST, IIO_ADC_EMU_CNVST_EN);
    if(ret) {
        dev_err(&st->spi->dev, "FAILED conversion reg write in handler");
        return IRQ_HANDLED;
    }

    for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->num_channels)
    {
        ret = iio_adc_emu_spi_read(st, IIO_ADC_EMU_REG_CHAN_HIGH(bit), &high);
        if(ret) {
            dev_err(&st->spi->dev, "FAILED read high in handler");
            return IRQ_HANDLED;
        }

        ret = iio_adc_emu_spi_read(st, IIO_ADC_EMU_REG_CHAN_LOW(bit), &low);
        if(ret) {
            dev_err(&st->spi->dev, "FAILED read low in handler");
            return IRQ_HANDLED;
        }

        buf[i] = (high << 8) | low;
        i++;
    }

    ret = iio_push_to_buffers(indio_dev, buf);
    if(ret) {
        dev_err(&st->spi->dev, "FAILED push to buffers");
        return IRQ_HANDLED;
    }
    iio_trigger_notify_done(indio_dev->trig);

    return IRQ_HANDLED;

}

static int iio_adc_emu_debugfs(struct iio_dev *indio_dev,
                                unsigned int reg, unsigned writeval,
                                unsigned int *readval)
 {
    struct iio_adc_emu_state *st = iio_priv(indio_dev);

    if(readval)
        return iio_adc_emu_spi_read(st, reg, (u8 *)readval);

    return iio_adc_emu_spi_write(st, reg, writeval);
 }                               

static const struct iio_info iio_adc_emu_info = {
    .read_raw = &iio_adc_emu_read_raw,
    .write_raw = &iio_adc_emu_write_raw,
    .debugfs_reg_access = &iio_adc_emu_debugfs,
}; 

static int iio_adc_emu_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    struct iio_adc_emu_state *st;
    int ret;

    indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
    if(!indio_dev)
        return -ENOMEM;
    
    indio_dev->name = "iio-adc-emu";
    indio_dev->info = &iio_adc_emu_info;
    indio_dev->channels = iio_adc_emu_chans;
    indio_dev->num_channels = ARRAY_SIZE(iio_adc_emu_chans);

    st = iio_priv(indio_dev);
    st->en = 0;
    st->chan0 = 0;
    st->chan1 = 0;
    st->spi = spi;

    ret = devm_iio_triggered_buffer_setup_ext(&spi->dev,
                                                indio_dev,     
                                                NULL, 
                                                iio_adc_emu_trig_handle,
                                                IIO_BUFFER_DIRECTION_IN,
                                                NULL,
                                                NULL);

    ret = iio_adc_emu_spi_write(st, IIO_ADC_EMU_REG_POWERON, 0);
    if (ret) {
        dev_err(&st->spi->dev, "Failed writting POWERON reg");
        return ret;
    }

    return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver iio_adc_emu_driver = {
    .driver = {
        .name = "iio-adc-emu",
    },
    .probe = iio_adc_emu_probe
};

module_spi_driver(iio_adc_emu_driver);


MODULE_AUTHOR("Alexa Alexandru");
MODULE_DESCRIPTION("Analog Device IIO EMU ADC Driver");
MODULE_LICENSE("GPL v2");