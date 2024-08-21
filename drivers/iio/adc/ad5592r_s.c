// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */

#include <asm/unaligned.h>
#include <linux/bitfield.h>

#include <linux/iio/buffer.h>
#include <linux/iio/iio.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>

#include <linux/module.h>
#include <linux/spi/spi.h>


#define AD5592R_S_WR_ADDR_MSK       GENMASK(14,11)
#define AD5592R_S_WR_VAL_MSK        GENMASK(8,0)
#define AD5592R_S_RDB_REG_SEL       GENMASK(5,2)
#define AD5592R_S_RDB_ENABLE        BIT(6)
#define AD5592R_S_CHAN_SEQ_SEL      GENMASK(7,0)
#define AD5592R_S_TEMP_SEQ_SEL      BIT(8)
#define AD5592R_S_SEQ_REP           BIT(9)
#define AD5592R_S_ADC_REG_SEL       GENMASK(7,0)
#define AD5592R_S_ADC_CHAN_SEL(x)   BIT(x)
#define AD5592R_S_ADC_DATA_RDB      GENMASK(11,0)
#define AD5592R_S_EN_REF            BIT(9)

#define AD5592R_S_CONF_RDB_REG      0x7
#define AD5592R_S_CONF_ADC_SEQ_REG  0x2
#define AD5592R_S_CONF_ADC_REG      0x4
#define AD5592R_S_CONF_REF_CTRL 0xB

struct ad5592r_s_state {
    struct spi_device *spi;
    bool en;
    int chann[7];
};

struct iio_chan_spec const ad5592r_s_chans[] = {
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
            .sign = 'u',
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
            .sign = 'u',
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
            .sign = 'u',
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
            .sign = 'u',
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
            .sign = 'u',
            .realbits = 12, 
            .storagebits = 16,
        }, 
    },
    //8 canale in datasheet, dar se folosesc 6
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 6,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .scan_index = 6,
        .scan_type = {
            .sign = 'u',
            .realbits = 12, 
            .storagebits = 16,
        }, 
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 7,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .scan_index = 7,
        .scan_type = {
            .sign = 'u',
            .realbits = 12, 
            .storagebits = 16,
        }, 
    }
};

//static pentru ca sunt functii interne




static int ad5592r_s_spi_nop(struct ad5592r_s_state *st,u16 *val)
{

    u16 tx = 0;
    struct spi_transfer xfer[] =
    {
      {
        .tx_buf = &tx,
        .rx_buf = val,
        .len = 2,
      }
    };

    return spi_sync_transfer(st->spi,xfer,1);
}

static int ad5592r_s_spi_write(struct ad5592r_s_state *st, u8 reg, u16 writeval)
{
    u16 tx = 0;
    u16 msg = 0;

    struct spi_transfer xfer[] =
    {
        { 
            .rx_buf = NULL,
            .tx_buf = &msg,
            .len = 2,
        }
    };

    tx = FIELD_PREP(AD5592R_S_WR_ADDR_MSK, reg) | FIELD_PREP(AD5592R_S_WR_VAL_MSK, writeval);
    //dev_info(&st->spi->dev, "tx = 0x%X", tx);
    
    
    put_unaligned_be16(tx,&msg);
    //dev_info(&st->spi->dev, "msg = 0x%X", msg);
    
    return spi_sync_transfer(st->spi, xfer, 1);
}



static int ad5592r_s_spi_read(struct ad5592r_s_state *st, u8 reg, u16 *readval)
{
    u16 tx = 0;
    u16 rx = 0;
    u16 msg = 0;
    int ret;
    struct spi_transfer xfer[] = 
    {
        {
            .rx_buf = NULL,
            .tx_buf = &msg,
            .len = 2,
        }
    };

    tx |= FIELD_PREP(AD5592R_S_WR_ADDR_MSK, AD5592R_S_CONF_RDB_REG);
    tx |= AD5592R_S_RDB_ENABLE;
    tx |= FIELD_PREP(AD5592R_S_RDB_REG_SEL, reg);

    //dev_info(&st->spi->dev, "tx = 0x%X", tx);

    put_unaligned_be16(tx,&msg);

    //dev_info(&st->spi->dev, "msg = 0x%X", msg);
    
    ret = spi_sync_transfer(st->spi,xfer,1);
    if(ret)
    {
        dev_err(&st->spi->dev,"Failed spi transfer config reg");
        return ret;
    }

    ret = ad5592r_s_spi_nop(st,&rx);
    if(ret)
    {
        dev_err(&st->spi->dev, "Failed nop transfer");
        return ret;
    }
    *readval = get_unaligned_be16(&rx);

    return 0;
}


static int ad5592r_s_read_chan(struct iio_chan_spec const *chan, int *val
                                ,struct ad5592r_s_state *st)
{
    //dev_info(&st->spi->dev,"Trying to read channel %d....",chan->channel);

    u16 tx = 0;
    u16 rx = 0;
    u16 msg = 0;
    int ret;
    struct spi_transfer xfer[] = 
    {
        {
            .rx_buf = NULL,
            .tx_buf = &msg,
            .len = 2,
        }
    };

    tx |= FIELD_PREP(AD5592R_S_WR_ADDR_MSK, AD5592R_S_CONF_ADC_SEQ_REG);
    tx |= FIELD_PREP(AD5592R_S_ADC_REG_SEL, AD5592R_S_ADC_CHAN_SEL(chan->channel));
    u8 high = 0;
    u8 low = 0;
    
    //dev_info(&st->spi->dev, "tx = 0x%X", tx);

    put_unaligned_be16(tx,&msg);

    //dev_info(&st->spi->dev, "msg = 0x%X", msg);
    
    ret = spi_sync_transfer(st->spi,xfer,1);
    if(ret)
    {
        dev_err(&st->spi->dev,"Failed spi transfer adc_seq reg");
        return ret;
    }
    
    ret = ad5592r_s_spi_nop(st,&rx);
    if(ret)
    {
        dev_err(&st->spi->dev, "Failed nop transfer");
        return ret;
    }
    dev_alert(&st->spi->dev,"First nop ended succesfully and returned %d",rx);
    ret = ad5592r_s_spi_nop(st,&rx);
    if(ret)
    {
        dev_err(&st->spi->dev, "Failed nop transfer");
        return ret;
    }
    dev_alert(&st->spi->dev,"Second nop ended succesfully and returned %d",rx);

    *val = get_unaligned_be16(&rx);
    //dev_info(&st->spi->dev,"Got data %d",*val);
    
    low = (u8)(*val & 0xF);
    high = (u8)((*val>>8) & 0xF);

    u16 aux = (high<<8) | low;
    //dev_info(&st->spi->dev,"High is %d and low is %d and aux is %d",high,low,aux);


    return FIELD_PREP(AD5592R_S_ADC_DATA_RDB,*val);
}

static int ad5592r_s_adc_pin_conf(struct iio_chan_spec const *chan,struct ad5592r_s_state *st,int val)
{
    if(val)
    {
        //dev_info(&st->spi->dev,"Trying to enable channels...");

        int ret = ad5592r_s_spi_write(st,AD5592R_S_CONF_ADC_REG,0xFF);
        if(ret)
        {
            dev_err(&st->spi->dev,"Failed spi write config reg for adc");
            return ret;
        }
    }
    else
    {
        //dev_info(&st->spi->dev,"Trying to disable channels...");

        int ret = ad5592r_s_spi_write(st,AD5592R_S_CONF_ADC_REG,0x0);
        if(ret)
        {
            dev_err(&st->spi->dev,"Failed spi write config reg for adc");
            return ret;
        }
    }
    
    return 0;
}

static int ad5592r_s_debugfs (struct iio_dev *indio_dev,
				  unsigned reg, unsigned writeval,
				  unsigned *readval)
{
    struct ad5592r_s_state*st = iio_priv(indio_dev);

    if(readval)
        return ad5592r_s_spi_read(st,reg,(u16 *)readval);
    
    return ad5592r_s_spi_write(st,reg,(u16)writeval);
}

static int ad5592r_s_read_raw (struct iio_dev *indio_dev,
	    struct iio_chan_spec const *chan,
		int *val,
		int *val2,
		long mask) 
{

    struct ad5592r_s_state *st = iio_priv(indio_dev);

    switch(mask) {
        case IIO_CHAN_INFO_ENABLE:
            *val = st->en;
            return IIO_VAL_INT;
        case IIO_CHAN_INFO_RAW:
            if (st->en) {
                *val = ad5592r_s_read_chan(chan,val,st);
                return IIO_VAL_INT;
            }
            else {
                return -EINVAL;
            }
            
        default:
            return -EINVAL;
    }

    return -EINVAL;
}

static int ad5592r_s_write_raw (struct iio_dev *indio_dev,
	    struct iio_chan_spec const *chan,
		int val,
		int val2,
		long mask) 
{

    struct ad5592r_s_state *st = iio_priv(indio_dev);

    switch(mask) {
        case IIO_CHAN_INFO_ENABLE:
            st->en = val;
            int ret = ad5592r_s_adc_pin_conf(chan,st,val);
            if(ret)
            {
                dev_err(&st->spi->dev,"Failed spi write config reg for adc");
                return ret;
            }
            //dev_info(&st->spi->dev,"Succsefully enabled the channels!");
            return 0;
        case IIO_CHAN_INFO_RAW:
            if (st->en) {
                st->chann[chan->channel] = val;
                return 0;
            }
            else {
                return -EINVAL;
            }
            
        default:
            return -EINVAL;
    }

    return -EINVAL;
}

static int ad5592r_s_spi_en_ref(struct ad5592r_s_state *st)
{
    u16 tx = 0;
    u16 rx = 0;
    u16 msg = 0;
    int ret;
    struct spi_transfer xfer[] = 
    {
        {
            .rx_buf = NULL,
            .tx_buf = &msg,
            .len = 2,
        }
    };

    tx |= FIELD_PREP(AD5592R_S_WR_ADDR_MSK, AD5592R_S_CONF_REF_CTRL);
    tx |= AD5592R_S_EN_REF;

   //dev_info(&st->spi->dev, "tx = 0x%X", tx);

    put_unaligned_be16(tx,&msg);

    //dev_info(&st->spi->dev, "msg = 0x%X", msg);
    
    ret = spi_sync_transfer(st->spi,xfer,1);
    if(ret)
    {
        dev_err(&st->spi->dev,"Failed spi internal ref config reg");
        return ret;
    }

    ret = ad5592r_s_spi_nop(st,&rx);
    if(ret)
    {
        dev_err(&st->spi->dev, "Failed nop transfer");
        return ret;
    }

    return 0;
}

static const struct iio_info ad5592r_s_info = {
    .read_raw = &ad5592r_s_read_raw,
    .write_raw = &ad5592r_s_write_raw,
    .debugfs_reg_access = &ad5592r_s_debugfs,
};

static irqreturn_t ad5592r_s_trig_handler(int irq, void *p) {
    //cate canale
    u16 buf[8];
    int bit = 0;
    struct iio_poll_func *pf = p;
    struct iio_dev *indio_dev = pf->indio_dev;

    //activarea conversiei
    int ret;
    struct ad5592r_s_state *st = iio_priv(indio_dev);

    //counter pentru numarul de canale active in buffer (daca unele canale nu sunt initalizate, nu mai trebuie sa apara in buffer)
    int i = 0;

    for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->num_channels) {
        //activarea canalului pentru conversie
        ret = ad5592r_s_spi_write(st, AD5592R_S_CONF_ADC_SEQ_REG, AD5592R_S_ADC_CHAN_SEL(bit));
        if (ret) {
            dev_err(&st->spi->dev, "Error during CONVERSION start in trigger handler");
            return ret;
        }

        u16 rx = 0;
        u16 temp = 0;
        ret = ad5592r_s_spi_nop(st, NULL);
        if (ret) {
            dev_err(&st->spi->dev, "Error during first NOP operation in trigger handler");
            return ret;
        }

        ret = ad5592r_s_spi_nop(st, &rx);
        if (ret) {
            dev_err(&st->spi->dev, "Error during getting conversion result in trigger handler");
            return ret;
        }

        put_unaligned_be16(rx, &temp);
        buf[i++] = FIELD_PREP(AD5592R_S_ADC_DATA_RDB, temp);

        //dev_info(&st->spi->dev, "Conversion result from trigger handler = 0x%X", buf[bit]);
    }

    ret = iio_push_to_buffers(indio_dev, buf);
    if (ret) {
        dev_err(&st->spi->dev, "Failed to push to buffers in trigger handler");
        return IRQ_HANDLED;
    }

    iio_trigger_notify_done(indio_dev->trig);

    return IRQ_HANDLED;
}



static int ad5592r_s_probe(struct spi_device *spi) {
    struct iio_dev *indio_dev;
    int ret;
    //variabila care va fi returnata ca succes/esec
    //int ret;

    indio_dev = devm_iio_device_alloc(&(spi->dev), 0);
    if (!indio_dev) {
        //cod de eroare standard de la Linux pentru cand nu se reuseste alocarea memoriei
        return -ENOMEM;
    }

    struct ad5592r_s_state *st = iio_priv(indio_dev);

    indio_dev->name = "ad5592r_s";
    indio_dev->info = &ad5592r_s_info;
    indio_dev->channels = ad5592r_s_chans;
    indio_dev->num_channels = ARRAY_SIZE(ad5592r_s_chans);

    ret = devm_iio_triggered_buffer_setup_ext(&spi->dev, indio_dev, NULL, ad5592r_s_trig_handler, IIO_BUFFER_DIRECTION_IN, NULL, NULL);
    if(ret)
    {
        dev_err(&st->spi->dev,"Failed to enable trigger ");
    }

    st = iio_priv(indio_dev);
    st->spi = spi;
    st->en = 0;
    ret = ad5592r_s_spi_en_ref(st);   
    if(ret)
    {
        dev_err(&st->spi->dev,"Failed to enable internal ref");
    }
    for(int i = 0 ;i < indio_dev->num_channels; i++)
    {
        st->chann[i] = 0;
    }
    
    
    return devm_iio_device_register(&spi->dev, indio_dev);
}


static struct spi_driver ad5592r_s_driver = {
    //numele driver-ului + functia cu care se probeaza
    .driver = {
        .name = "ad5592r_s",
    },
    .probe = &ad5592r_s_probe
};

module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Simonca Raul <simonca.raul@gmail.com>");
MODULE_DESCRIPTION("Analog Devices AD5592R");
MODULE_LICENSE("GPL v2");