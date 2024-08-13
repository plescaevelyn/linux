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


#define IIO_ADC_EMU_READ_MSK            BIT(7)
#define IIO_ADC_EMU_READ_ADDR_MSK       GENMASK(6, 0)

#define IIO_ADC_EMU_WRITE_ADDR_MSK      GENMASK(14, 8)
#define IIO_ADC_EMU_WRITE_DATA_MSK      GENMASK(7, 0)

#define IIO_ADC_EMU_REG_CNVST           0X3
#define IIO_ADC_EMU_REG_POWERON         0X2

#define IIO_ADC_EMU_REG_CHAN_HIGH(x)    0X4 + x*2
#define IIO_ADC_EMU_REG_CHAN_LOW(x)     0X5 + x*2
#define IIO_ADC_EMU_REG_CHIP_ID         0X0

#define IIO_ADC_EMU_CNVST_EN            BIT(0)

//necesara pentru stocarea datelor interne din cadrul chip-ului
struct iio_adc_emu_state {
    bool en;
    int chan0;
    int chan1;

    //structura pentru spi
    struct spi_device *spi;
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
    }
};

//static pentru ca sunt functii interne
static int iio_adc_emu_spi_read(struct iio_adc_emu_state *st, u8 *readval, u8 reg) {
    u8 tx;
    u8 rx;
    int ret;
    
    struct spi_transfer xfer[] = {
        {
            //primul transfer -> adresa 
            //NULL pentru ca nu se returneaza nimic initial la transfer
            .rx_buf = NULL,
            .tx_buf = &tx,
            .len = 1,
        },
        {
            //al doilea transfer -> citirea datelor 
            .rx_buf = &rx,
            .tx_buf = NULL,
            .len = 1,
        }
    };

    tx = IIO_ADC_EMU_READ_MSK | FIELD_PREP(IIO_ADC_EMU_READ_ADDR_MSK, reg); 

    ret = spi_sync_transfer(st->spi, xfer, 2);
    if (ret) {
        dev_err(&st->spi->dev, "Spi sync transfer failed during read");
        return ret;
    }

    *readval = rx;
    return 0;
}

static int iio_adc_emu_spi_write(struct iio_adc_emu_state *st, u8 reg, u8 writeval) {
    //u8 tx[2];
    u16 tx = 0;
    u16 msg = 0;
    int ret;
    
    struct spi_transfer xfer = {
            .rx_buf = NULL,
            .tx_buf = &msg,
            .len = 2,
    };

    //bit-ul de write e 0, deci nu e necesara o masca, DAR e obligatorie initializarea lui tx cu 0
    tx = FIELD_PREP(IIO_ADC_EMU_WRITE_ADDR_MSK, reg) | 
         FIELD_PREP(IIO_ADC_EMU_WRITE_DATA_MSK, writeval); 

    dev_info(&st->spi->dev, "tx = 0x%X", tx);

    put_unaligned_be16(tx, &msg);
    dev_info(&st->spi->dev, "msg = 0x%X", msg);

    ret = spi_sync_transfer(st->spi, &xfer, 1);
    if (ret) {
        dev_err(&st->spi->dev, "Spi sync transfer failed during write");
        return ret;
    }

    return 0;
}


static int iio_adc_emu_read_chan(struct iio_adc_emu_state *st, struct iio_chan_spec *chan, int *val) {
    int ret;
    ret = iio_adc_emu_spi_write(st, IIO_ADC_EMU_REG_CNVST, IIO_ADC_EMU_CNVST_EN);

    if (ret) {
        dev_err(&st->spi->dev, "Failed conversion reg write");
        return ret;
    }

    u8 high;
    u8 low;

    ret = iio_adc_emu_spi_read(st, &high,IIO_ADC_EMU_REG_CHAN_HIGH(chan->channel));
    if (ret) {
        dev_err(&st->spi->dev, "Failed to read high register");
        return ret;
    }

    ret = iio_adc_emu_spi_read(st, &low,IIO_ADC_EMU_REG_CHAN_LOW(chan->channel));
    if (ret) {
        dev_err(&st->spi->dev, "Failed to read low register");
        return ret;
    }

    *val = (high << 8) | low;
    return 0;   
}

static int iio_adc_emu_write_raw (struct iio_dev *indio_dev,
	    struct iio_chan_spec const *chan,
		int val,
		int val2,
		long mask) 
{

    struct iio_adc_emu_state *st = iio_priv(indio_dev);

    switch(mask) {
        //canalele sunt read-only!!!
        case IIO_CHAN_INFO_ENABLE:
            st->en = val;
            return 0;
        default:
            return -EINVAL;
    }

    return -EINVAL;
}

static int iio_adc_emu_read_raw (struct iio_dev *indio_dev,
	    struct iio_chan_spec const *chan,
		int *val,
		int *val2,
		long mask) 
{

    struct iio_adc_emu_state *st = iio_priv(indio_dev);
    int ret;
    
    switch(mask) {
        case IIO_CHAN_INFO_RAW:
            if(st->en) {
                ret = iio_adc_emu_read_chan(st, chan, val);
                if (ret) {
                    dev_err(&st->spi->dev, "Error reading from channel");
                    return ret;
                }
            return IIO_VAL_INT;
            }
            else {
                return -EINVAL;
            }
        case IIO_CHAN_INFO_ENABLE:
            *val = st->en;
            return IIO_VAL_INT;            
        default:
            return -EINVAL;
    }

    return -EINVAL;
}

static irqreturn_t iio_adc_emu_trig_handler(int irq, void *p) {
    //cate canale
    u16 buf[2];
    int bit = 0;
    struct iio_poll_func *pf = p;
    struct iio_dev *indio_dev = pf->indio_dev;

    //activarea conversiei
    int ret;
    struct iio_adc_emu_state *st = iio_priv(indio_dev);
    ret = iio_adc_emu_spi_write(st, IIO_ADC_EMU_REG_CNVST, IIO_ADC_EMU_CNVST_EN);
    if (ret) {
        dev_err(&st->spi->dev, "Failed conversion reg write during trigger handling");
        return IRQ_HANDLED;
    }

    u8 high;
    u8 low;    
    //counter pentru numarul de canale active in buffer (daca unele canale nu sunt initalizate, nu mai trebuie sa apara in buffer)
    int i = 0;

    for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->num_channels) {
        ret = iio_adc_emu_spi_read(st, &high, IIO_ADC_EMU_REG_CHAN_HIGH(bit));
        if (ret) {
            dev_err(&st->spi->dev, "Failed to read high register during trigger handling");
            return IRQ_HANDLED;
        }

        ret = iio_adc_emu_spi_read(st, &low, IIO_ADC_EMU_REG_CHAN_LOW(bit));
        if (ret) {
            dev_err(&st->spi->dev, "Failed to read low register during trigger handling");
            return IRQ_HANDLED;
        }

        buf[i++] = (high << 8) | low;  
    }

    ret = iio_push_to_buffers(indio_dev, buf);
    if (ret) {
        dev_err(&st->spi->dev, "Failed to push to buffers in trigger handler");
        return IRQ_HANDLED;
    }

    iio_trigger_notify_done(indio_dev->trig);

    return IRQ_HANDLED;
}

static int iio_adc_emu_reg_access(struct iio_dev *indio_dev,
		unsigned reg, 
        unsigned writeval,
	    unsigned *readval) {

    struct iio_adc_emu_state *st = iio_priv(indio_dev);
    if (readval) {
        return iio_adc_emu_spi_read(st, (u8*)readval, reg);
    }
    
    return iio_adc_emu_spi_write(st, reg, writeval);

    //return (readval != NULL) ? iio_adc_emu_spi_read(st, (u8*)readval, reg) : iio_adc_emu_spi_write(st, reg, writeval);
}


static const struct iio_info iio_adc_emu_info = {
   .read_raw = &iio_adc_emu_read_raw,
   .write_raw = &iio_adc_emu_write_raw,
   .debugfs_reg_access = &iio_adc_emu_reg_access,
};

static int iio_adc_emu_probe(struct spi_device *spi) {
    struct iio_dev *indio_dev;
    struct iio_adc_emu_state *st;
    //variabila care va fi returnata ca succes/esec
    int ret;

    indio_dev = devm_iio_device_alloc(&(spi->dev), sizeof(*st));
    if (!indio_dev) {
        //cod de eroare standard de la Linux pentru cand nu se reuseste alocarea memoriei
        return -ENOMEM;
    }

    
    indio_dev->name = "iio-adc-emu";
    indio_dev->info = &iio_adc_emu_info;
    indio_dev->channels = iio_adc_emu_chans;
    indio_dev->num_channels = ARRAY_SIZE(iio_adc_emu_chans);

    st = iio_priv(indio_dev);
    st->en = 0;   
    st->chan0 = 0;
    st->chan1 = 0; 
    st->spi = spi;

    ret = devm_iio_triggered_buffer_setup_ext(&spi->dev, indio_dev, NULL, iio_adc_emu_trig_handler, IIO_BUFFER_DIRECTION_IN, NULL, NULL);
    if (ret) {
        dev_err(&st->spi->dev, "Error during buffer setup");
        return ret;
    }

    ret = iio_adc_emu_spi_write(st, IIO_ADC_EMU_REG_POWERON, 0);
    if (ret) {
        dev_err(&st->spi->dev, "Error while powering on ADC");
        return ret;
    }

    return devm_iio_device_register(&spi->dev, indio_dev);
}


static struct spi_driver iio_adc_emu_driver = {
    //numele driver-ului + functia cu care se probeaza
    .driver = {
        .name = "iio-adc-emu",
    },
    .probe = &iio_adc_emu_probe
};

module_spi_driver(iio_adc_emu_driver);

MODULE_AUTHOR("Volcov Sabina <volcovsabina@gmail.com>");
MODULE_DESCRIPTION("Analog Devices IIO ADC Emulator");
MODULE_LICENSE("GPL v2");