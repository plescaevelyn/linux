// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <asm/unaligned.h>
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/bitfield.h>

#include <linux/iio/buffer.h>
#include <linux/iio/iio.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>

#define AD5592R_S_WR_ADDR_MSK GENMASK(14,11)
#define AD5592R_S_WR_DATA_MSK GENMASK(9,0)
#define AD5592R_S_RDB_REG_SEL GENMASK(5,2)
#define AD5592R_S_RDB_EN BIT(6)

#define AD5592R_S_CONF_RDB_REG 0X7

#define AD5592R_S_ADC_SEQ_ADR       0x2
#define AD5592R_S_ADC_SEQ_CHAN_EN(y)    BIT(y)
#define AD5592R_S_ADC_DATA          GENMASK(11,0)
#define AD5592R_S_PD_REF_CTRL_ADR   0xb
#define AD5592R_S_PD_REF_CTRL_EN    BIT(9)
#define AD5592R_S_ADC_CONFIG_ADR    0x4
#define AD5592R_S_ADC_CONFIG_CHAN_EN GENMASK(5,0)

struct ad5592r_s_state {
    struct spi_device *spi;
    bool en;
    int chan[6];
};

struct iio_chan_spec const ad5592r_s_chans[]={
    {
        .type = IIO_VOLTAGE,
        .indexed = 1, //daca vreau sa numerotez canalele
        .channel = 0, //numar canal
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
        .indexed = 1, //daca vreau sa numerotez canalele
        .channel = 1, //numar canal
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
        .indexed = 1, //daca vreau sa numerotez canalele
        .channel = 2, //numar canal
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
        .indexed = 1, //daca vreau sa numerotez canalele
        .channel = 3, //numar canal
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
        .indexed = 1, //daca vreau sa numerotez canalele
        .channel = 4, //numar canal
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
        .indexed = 1, //daca vreau sa numerotez canalele
        .channel = 5, //numar canal
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .scan_index = 5,
        .scan_type = {
            .sign = 'u',
            .realbits = 12,
            .storagebits = 16,
        }
    }
};

static int ad5592r_s_spi_write(struct ad5592r_s_state *st, u8 reg, u16 writeval)
{
    u16 tx = 0;
    u16 msg = 0;

    struct spi_transfer xfer[]={
        {
            .tx_buf=&msg,
            .rx_buf=NULL,
            .len=2,
        }
    };

    tx = FIELD_PREP(AD5592R_S_WR_ADDR_MSK,reg) | FIELD_PREP(AD5592R_S_WR_DATA_MSK,writeval);

    //dev_info(&st->spi->dev, "tx = 0x%X", tx);

    put_unaligned_be16(tx,&msg);
    //dev_info(&st->spi->dev, "msg = 0x%X", msg);

    return spi_sync_transfer(st->spi,xfer,1);
}

static int ad5592r_s_spi_nop(struct ad5592r_s_state *st, u16 *val)
{
    u16 tx = 0;
    struct spi_transfer xfer[] = {
        {
            .tx_buf = &tx,
            .rx_buf = val,
            .len = 2,
        }
    };

    return spi_sync_transfer(st->spi,xfer,1);
}

static int ad5592r_s_spi_read(struct ad5592r_s_state *st, u8 reg, u16 *readval)
{
    u16 tx = 0;
    u16 rx = 0;
    int ret;
    u16 msg = 0;

    struct spi_transfer xfer[] = {
        {
            .rx_buf = NULL,
            .tx_buf = &msg,
            .len = 2,
        }
    };
    
    tx |= FIELD_PREP(AD5592R_S_WR_ADDR_MSK,AD5592R_S_CONF_RDB_REG);
    tx |= AD5592R_S_RDB_EN;
    tx |= FIELD_PREP(AD5592R_S_RDB_REG_SEL, reg);

    //dev_info(&st->spi->dev, "tx = 0x%X", tx);

    put_unaligned_be16(tx,&msg);

    //dev_info(&st->spi->dev, "msg = 0x%X", msg);
    
    ret = spi_sync_transfer(st->spi,xfer,1);
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

static int ad5592r_s_read_chann(struct ad5592r_s_state *st, struct iio_chan_spec const *chan, int *val)
{
    int ret;
    u16 writev = 0;
    writev = AD5592R_S_ADC_SEQ_CHAN_EN(chan->channel);
    //dev_info(&st->spi->dev, "msgtow = 0x%X", writev);

    ret = ad5592r_s_spi_write(st, AD5592R_S_ADC_SEQ_ADR, writev);
    if(ret){
        dev_err(&st->spi->dev, "FAILED writing ADC Sequence reg");
        return ret;
    }

    u16 invalid_data = 0;
    u16 rx = 0;
    u16 temp = 0;
    ret = ad5592r_s_spi_nop(st,&invalid_data);
    ret = ad5592r_s_spi_nop(st,&rx);
    //dev_info(&st->spi->dev, "rx = 0x%X", rx);
    if(ret){
        dev_err(&st->spi->dev, "FAILED read chan");
        return ret;
    }
    temp = get_unaligned_be16(&rx); 
    temp = AD5592R_S_ADC_DATA & temp;
    
    //dev_info(&st->spi->dev, "temp_value = %d", temp);
    *val = temp;

    return 0;
}

static int ad5592r_s_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val,
			int *val2,
			long mask)
{
   struct ad5592r_s_state *st = iio_priv(indio_dev);
   int ret;

    switch(mask){
        case IIO_CHAN_INFO_RAW:
            if(st->en){
                ret = ad5592r_s_read_chann(st,chan, val);
                if(ret){
                    dev_err(&st->spi->dev, "Error reading from channel");
                    return ret;
                }
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

static int ad5592r_s_write_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int val,
			int val2,
			long mask)
{
    struct ad5592r_s_state *st = iio_priv(indio_dev);
    switch(mask){
        case IIO_CHAN_INFO_ENABLE:
            st->en = val;
            return 0;
        default:
            return -EINVAL;
    }

    return -EINVAL;  
}

static irqreturn_t ad5592r_s_trig_handler(int irq, void *p)
{
    struct iio_poll_func *pf = p;
    struct iio_dev *indio_dev = pf->indio_dev;
    struct ad5592r_s_state *st = iio_priv(indio_dev);
    u16 buf[6];
    int bit = 0;
    int i = 0;
    int ret;
    int res;
    
    for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->num_channels){

        ret = ad5592r_s_read_chann(st,&indio_dev->channels[bit], &res);

        buf[i] = res;
        i++;
    }

    ret = iio_push_to_buffers(indio_dev, buf);
    if(ret){
        dev_err(&st->spi->dev, "FAILED push to buffer");
        iio_trigger_notify_done(indio_dev->trig);
        return IRQ_HANDLED;
    }

    iio_trigger_notify_done(indio_dev->trig);

    return IRQ_HANDLED;
}

static int ad5592r_s_debugfs(struct iio_dev *indio_dev,
				  unsigned reg, unsigned writeval,
				  unsigned *readval)
{
    struct ad5592r_s_state *st = iio_priv(indio_dev);

    if(readval)
        return ad5592r_s_spi_read(st,reg,(u16 *)readval);

    return ad5592r_s_spi_write(st,reg,writeval);
}

static const struct iio_info ad5592r_s_info = {
     .read_raw=&ad5592r_s_read_raw,
     .write_raw=&ad5592r_s_write_raw,
     .debugfs_reg_access = &ad5592r_s_debugfs,
};

static int ad5592r_s_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    struct ad5592r_s_state *st;
    int ret;

    indio_dev = devm_iio_device_alloc(&spi->dev,sizeof(*st));
    if(!indio_dev)
        return -ENOMEM; //cod de eroare out of memory

    indio_dev->name="ad5592r_s";
    indio_dev->info = &ad5592r_s_info;
    indio_dev->channels = ad5592r_s_chans;
    indio_dev->num_channels = ARRAY_SIZE(ad5592r_s_chans);

    st = iio_priv(indio_dev);
    st->en = 0;
    for(int i=0;i<6;i++)
        st->chan[i]=0;
    st->spi = spi;

    ret = devm_iio_triggered_buffer_setup_ext(&spi->dev, indio_dev, NULL, ad5592r_s_trig_handler, IIO_BUFFER_DIRECTION_IN, NULL, NULL);

    ret = ad5592r_s_spi_write(st, AD5592R_S_PD_REF_CTRL_ADR, AD5592R_S_PD_REF_CTRL_EN);
    if(ret){
        dev_err(&spi->dev, "Failed writing PD_REF_CTRL reg");
        return ret;
    }

    ret = ad5592r_s_spi_write(st, AD5592R_S_ADC_CONFIG_ADR, AD5592R_S_ADC_CONFIG_CHAN_EN);
    if(ret){
        dev_err(&spi->dev, "Failed writing ADC_CONFIG reg");
        return ret;
    }

    return devm_iio_device_register(&spi->dev,indio_dev);
};

static struct spi_driver ad5592r_s_driver = {
    .driver = {
        .name = "ad5592r_s"
    },
    .probe = ad5592r_s_probe
};
module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Elena Hadarau <elena.hadarau@yahoo.com>");
MODULE_DESCRIPTION("Analog Devices ad5592r_s");
MODULE_LICENSE("GPL v2");