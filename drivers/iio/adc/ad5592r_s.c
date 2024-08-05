// SPDX-License-Identifier: GPL-2.0-only
/*
 * Analog Devices AD9467 SPI ADC driver
 
 * Copyright 2024 Analog Devices Inc.
 */
#include<linux/spi/spi.h>
#include<linux/module.h>
#include<linux/iio/iio.h>

struct iio_chan_spec const iio_ad5592r_s_chans[]={
   { .type = IIO_VOLTAGE,
      .indexed=1,
      .channel =0,
      .info_mask_separate=BIT(IIO_CHAN_INFO_RAW),
   },
    {
      .type = IIO_VOLTAGE,
      .indexed=1,
      .channel =1,
      .info_mask_separate=BIT(IIO_CHAN_INFO_RAW),
   },
      { .type = IIO_VOLTAGE,
      .indexed=1,
      .channel =2,
      .info_mask_separate=BIT(IIO_CHAN_INFO_RAW),
   },
    {
      .type = IIO_VOLTAGE,
      .indexed=1,
      .channel =3,
      .info_mask_separate=BIT(IIO_CHAN_INFO_RAW),
   },
    {
      .type = IIO_VOLTAGE,
      .indexed=1,
      .channel =4,
      .info_mask_separate=BIT(IIO_CHAN_INFO_RAW),
   },
   {
      .type = IIO_VOLTAGE,
      .indexed=1,
      .channel =5,
      .info_mask_separate=BIT(IIO_CHAN_INFO_RAW),
   }
};
static	int iio_ad5592r_s_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val,
			int *val2,
			long mask)
            {
             switch (mask)
             {
             case IIO_CHAN_INFO_RAW:
                switch(chan->channel)
                { case 0: 
                    *val =87;
                    return IIO_VAL_INT;
                   case 1:  
                    *val =87;
                    return IIO_VAL_INT;
                   case 2:  
                    *val =28;
                    return IIO_VAL_INT;
                   case 3:  
                    *val =500;
                    return IIO_VAL_INT;
                   case 4:  
                    *val =1966;
                    return IIO_VAL_INT;
                   case 5:  
                    *val =2001;
                    return IIO_VAL_INT;
                }
             default:
                return -EINVAL;
             }   
             return -EINVAL;
            }
static const struct iio_info iio_ad5592r_s_info={
   .read_raw=&iio_ad5592r_s_read_raw,
};
static int iio_ad5592r_s_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;
    int ret;
    indio_dev= devm_iio_device_alloc(&spi->dev, 0);
    if(!indio_dev)
         return ENOMEM;
    indio_dev->name ="ad5592r_s";
    indio_dev->info = &iio_ad5592r_s_info;

     indio_dev->channels=iio_ad5592r_s_chans;
    indio_dev->num_channels= ARRAY_SIZE(iio_ad5592r_s_chans);
    return devm_iio_device_register(&spi->dev, indio_dev);

}
static struct spi_driver iio_ad5592r_s_driver={
    .driver={
        .name="ad5592r_s"
    },
    .probe= iio_ad5592r_s_probe
};


module_spi_driver(iio_ad5592r_s_driver);
MODULE_AUTHOR("Rosca David-Sorin ");
MODULE_DESCRIPTION("Analog Devices IIO AD5592R driver");
MODULE_IMPORT_NS("GPL v2");