// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Analog Devices, Inc.
 */


#include <linux/bitfield.h>
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>
#include <asm/unaligned.h>

#define AD5592R_S_READBACK_ENABLE       BIT(6)
#define AD5592R_S_READBACK_REG_SEL      GENMASK(5, 2)
#define AD5592R_S_CONF_READBACK_REG     0x7

#define AD5592R_S_WRITE_ADDR_MSK        GENMASK(14, 11)
#define AD5592R_S_WRITE_DATA_MSK        GENMASK(9, 0)

#define AD5592R_S_REF_EN                BIT(9)
#define AD5592R_S_CONFIG_MSK            GENMASK(5, 0)
#define AD5592R_S_SEQ_MSK(x)            BIT(x)
#define AD5592R_S_PD_REG                0xB
#define AD5592R_S_ADC_CONF_REG          0x4
#define AD5592R_S_ADC_SEQ_REG           0x2
#define AD5592R_S_CONV_RES_MSK          GENMASK(11, 0)



struct ad5592r_s_state {
    bool en;
    int chans[8];

    struct spi_device *spi;
};

struct iio_chan_spec const ad5592r_s_chans[] = {
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
    //8 canale in datasheet, dar se folosesc 6
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 6,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 7,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    }
};

//functie care va "sta"; ii ia 2 transferuri ADC-ului pentru a lua datele bune
static int ad5592r_s_spi_nop(struct ad5592r_s_state *st, u16 *val) {
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


//static pentru ca sunt functii interne
static int ad5592r_s_spi_read(struct ad5592r_s_state *st, u16 *readval, u8 reg) {
    u16 tx = 0;
    u16 msg = 0;
    u16 rx = 0;
    int ret;
    
    struct spi_transfer xfer[] = {
        {
            //primul transfer -> configurare pentru readback
            //NULL pentru ca nu se returneaza nimic initial la transfer
            .rx_buf = NULL,
            .tx_buf = &msg,
            .len = 2,
        },
    };

    tx =  FIELD_PREP(AD5592R_S_WRITE_ADDR_MSK, AD5592R_S_CONF_READBACK_REG)
        | AD5592R_S_READBACK_ENABLE 
        | FIELD_PREP(AD5592R_S_READBACK_REG_SEL, reg);

    //scriere preliminara pentru SPI pentru a putea face readback
    put_unaligned_be16(tx, &msg);

    dev_info(&st->spi->dev, "tx = 0x%X", tx);
    dev_info(&st->spi->dev, "msg = 0x%X", msg);

    ret = spi_sync_transfer(st->spi, xfer, 1);
    if (ret) {
        dev_err(&st->spi->dev, "Spi sync transfer failed during readback config");
        return ret;
    }

    ret = ad5592r_s_spi_nop(st, &rx);
    if (ret) {
        dev_err(&st->spi->dev, "Failed during nop transfer");
        return ret;
    }

    *readval = get_unaligned_be16(&rx);
    return 0;
}

static int ad5592r_s_spi_write(struct ad5592r_s_state *st, u8 reg, u16 writeval) {
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
    tx = FIELD_PREP(AD5592R_S_WRITE_ADDR_MSK, reg) | 
         FIELD_PREP(AD5592R_S_WRITE_DATA_MSK, writeval); 

    dev_info(&st->spi->dev, "tx = 0x%X", tx);

    //little endian de la linux, dar trebuie sa primim datele ca big endian
    put_unaligned_be16(tx, &msg);
    dev_info(&st->spi->dev, "msg = 0x%X", msg);

    ret = spi_sync_transfer(st->spi, &xfer, 1);
    if (ret) {
        dev_err(&st->spi->dev, "Spi sync transfer failed during write");
        return ret;
    }

    return 0;
}


static int ad5592r_s_read_chan(struct ad5592r_s_state *st, struct iio_chan_spec *chan, int *val) {
    int ret;

    //activarea canalului pentru conversie
    ret = ad5592r_s_spi_write(st, AD5592R_S_ADC_SEQ_REG, AD5592R_S_SEQ_MSK(chan->channel));
    if (ret) {
        dev_err(&st->spi->dev, "Error during CONVERSION start");
        return ret;
    }

    u16 rx = 0;
    u16 temp = 0;
    ret = ad5592r_s_spi_nop(st, NULL);
    if (ret) {
        dev_err(&st->spi->dev, "Error during first NOP operation");
        return ret;
    }

    ret = ad5592r_s_spi_nop(st, &rx);
    if (ret) {
        dev_err(&st->spi->dev, "Error during getting conversion result");
        return ret;
    }

    put_unaligned_be16(rx, &temp);
    *val = FIELD_PREP(AD5592R_S_CONV_RES_MSK, temp);

    dev_info(&st->spi->dev, "Conversion result = 0x%X", *val);
    return ret;
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
            return 0;
        //canalele sunt read-only!!
        /* 
        case IIO_CHAN_INFO_RAW:
            if (st->en) {
                st->chans[chan->channel] = val;
                return 0;
            }
            else {
                return -EINVAL;
            }
        */
        default:
            return -EINVAL;
    }

    return -EINVAL;
}

static int ad5592r_s_read_raw (struct iio_dev *indio_dev,
	    struct iio_chan_spec const *chan,
		int *val,
		int *val2,
		long mask) 
{

    struct ad5592r_s_state *st = iio_priv(indio_dev);
    int ret;

    switch(mask) {
        case IIO_CHAN_INFO_ENABLE:
            *val = st->en;
            return IIO_VAL_INT;
        case IIO_CHAN_INFO_RAW:
            if (st->en) {
                //*val = st->chans[chan->channel];
                ret = ad5592r_s_read_chan(st, chan, val);
                if (ret) {
                    dev_err(&st->spi->dev, "Error reading from ADC channel");
                    return ret;
                }
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

static int ad5592r_s_reg_access(struct iio_dev *indio_dev,
		unsigned reg, 
        unsigned writeval,
	    unsigned *readval) {


    struct ad5592r_s_state *st = iio_priv(indio_dev);
    if (readval) {
        return ad5592r_s_spi_read(st, (u16*)readval, reg);
    }
    
    return ad5592r_s_spi_write(st, reg, writeval);
    //return readval ? ad5592r_s_spi_read(st, (u8*)readval, reg) : ad5592r_s_spi_write(st, reg, writeval);
}


static const struct iio_info ad5592r_s_info = {
    .read_raw = &ad5592r_s_read_raw,
    .write_raw = &ad5592r_s_write_raw,
    .debugfs_reg_access = &ad5592r_s_reg_access,
};

static int ad5592r_s_probe(struct spi_device *spi) {
    struct iio_dev *indio_dev;
    //variabila care va fi returnata ca succes/esec
    int ret;

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

    st = iio_priv(indio_dev);
    //activare automata ?
    st->en = 1;  
    for (int i = 0; i < 8; i++) { 
        st->chans[i] = 0;
    }
    st->spi = spi;

    //enable pentru referinta interna
    ret = ad5592r_s_spi_write(st, AD5592R_S_PD_REG, AD5592R_S_REF_EN);
    if (ret) {
        dev_err(&st->spi->dev, "Error during REF initialization");
        return ret;
    }

    //activarea regimului ADC pentru 6 canale
    ret = ad5592r_s_spi_write(st, AD5592R_S_ADC_CONF_REG, AD5592R_S_CONFIG_MSK);
    if (ret) {
        dev_err(&st->spi->dev, "Error during CONFIG REG initialization");
        return ret;
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

MODULE_AUTHOR("Volcov Sabina <volcovsabina@gmail.com>");
MODULE_DESCRIPTION("Analog Devices AD5592R");
MODULE_LICENSE("GPL v2");