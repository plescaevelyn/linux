// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <asm/unaligned.h>
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>
#include <linux/bitfield.h>

#define IIO_ADC_EMU_READ_MSK BIT(7)
#define IIO_ADC_EMU_ADDR_READ_MSK GENMASK(6,0)

#define IIO_ADC_EMU_ADDR_WRITE_MSK GENMASK(14,8)
#define IIO_ADC_EMU_WRITEVAL_MSK GENMASK(7,0)


struct iio_adc_emu_state {
    struct spi_device *spi;
    bool en;
    int chan0;
    int chan1;
};

struct iio_chan_spec const iio_adc_emu_chans[]={
    {
        .type = IIO_VOLTAGE,
        .indexed = 1, //daca vreau sa numerotez canalele
        .channel = 0, //numar canal
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1, //daca vreau sa numerotez canalele
        .channel = 1, //numar canal
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
    }
};

static int iio_adc_emu_spi_read(struct iio_adc_emu_state *st, u8 reg, u8 *readval)
{
    //u8 reg - adresa
    u8 tx;
    u8 rx;
    int ret; //pentru a verifica codul de eroare
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

    tx = IIO_ADC_EMU_READ_MSK | FIELD_PREP(IIO_ADC_EMU_ADDR_READ_MSK,reg);

    ret = spi_sync_transfer(st->spi,xfer,2);
    if(ret){
        dev_err(&st->spi->dev, "SPI sync transfer fail during read");
        return ret;
    }

    *readval = rx;

    return 0;
}

static int iio_adc_emu_spi_write(struct iio_adc_emu_state *st, u8 reg, u8 writeval)
{
    //u8 reg - adresa
    u16 tx=0;
    u16 msg=0;
    struct spi_transfer xfer = {
        .rx_buf = NULL,
        .tx_buf = &msg,
        .len = 2, 
    };

    tx = FIELD_PREP(IIO_ADC_EMU_ADDR_WRITE_MSK,reg) | FIELD_PREP(IIO_ADC_EMU_WRITEVAL_MSK,writeval);

    dev_info(&st->spi->dev, "tx = 0x%X", tx);

    put_unaligned_be16(tx,&msg);
    dev_info(&st->spi->dev, "msg = 0x%X", msg);

    return spi_sync_transfer(st->spi,&xfer,1);
}

static int iio_adc_emu_write_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int val,
			int val2,
			long mask)
{
    struct iio_adc_emu_state *st = iio_priv(indio_dev);
    switch(mask){
        case IIO_CHAN_INFO_RAW:
            if(st->en){
                if(chan->channel)
                    st->chan1=val;
                else
                    st->chan0=val;
                return 0;
            }
            else
                return -EINVAL;
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
    struct iio_adc_emu_state *st = iio_priv(indio_dev);
    switch(mask){
        case IIO_CHAN_INFO_RAW:
            if(st->en){
                if(chan->channel)
                    *val=st->chan1;
                else
                    *val=st->chan0;
                return IIO_VAL_INT;
            }
            else
                return -EINVAL;
        case IIO_CHAN_INFO_ENABLE:
            *val=st->en;
            return IIO_VAL_INT;
        default:
            return -EINVAL;
    }

    return -EINVAL;  
}

static int iio_adc_emu_debugfs(struct iio_dev *indio_dev,
				  unsigned reg, unsigned writeval,
				  unsigned *readval)
{
    struct iio_adc_emu_state *st = iio_priv(indio_dev);

    if(readval)
        return iio_adc_emu_spi_read(st,reg,(u8 *)readval);

    return iio_adc_emu_spi_write(st,reg,writeval);
}

static const struct iio_info iio_adc_emu_info = {
    .read_raw=&iio_adc_emu_read_raw,
    .write_raw=&iio_adc_emu_write_raw,
    .debugfs_reg_access = &iio_adc_emu_debugfs,
};

static int iio_adc_emu_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    struct iio_adc_emu_state *st;
    // int ret;

    indio_dev = devm_iio_device_alloc(&spi->dev,sizeof(*st));
    if(!indio_dev)
        return -ENOMEM; //cod de eroare out of memory

    indio_dev->name="iio-adc-emu";
    indio_dev->info = &iio_adc_emu_info;
    indio_dev->channels = iio_adc_emu_chans;
    indio_dev->num_channels = ARRAY_SIZE(iio_adc_emu_chans);

    st = iio_priv(indio_dev);
    st->en = 0;
    st->chan0=0;
    st->chan1=0;
    st->spi=spi;

    return devm_iio_device_register(&spi->dev,indio_dev);
}

static struct spi_driver iio_adc_emu_driver = {
    .driver = {
        .name = "iio-adc-emu"
    },
    .probe = iio_adc_emu_probe
};
module_spi_driver(iio_adc_emu_driver);

MODULE_AUTHOR("Elena Hadarau <elena.hadarau@yahoo.com>");
MODULE_DESCRIPTION("Analog Devices ADC Emulator");
MODULE_LICENSE("GPL v2");