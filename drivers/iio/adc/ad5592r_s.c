#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>

static int ad5592r_s_probe(struct spi_device*);
static int ad5592r_s_read_raw(struct iio_dev*, struct iio_chan_spec const*, int*, int*, long);

/**
 * IIO_INFO structure
 */
static const struct iio_info ad5592r_s_info = {
    .read_raw = &ad5592r_s_read_raw,
};


/**
 * Driver structure
 */
static struct spi_driver ad5592r_s_driver =
{
    .driver = {
        .name = "ad5592r_s",
    },
    .probe = ad5592r_s_probe
};

/**
 *  Channels specification structure 
 */
static const struct iio_chan_spec ad5592r_s_channels[] = {
    {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .indexed = 1,
        .channel = 0,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .indexed = 1,
        .channel = 1,
    }
};

/**
 * Function to read the raw value of the adi_emu channels
 * indio_dev    - the created iio-adi-emu device
 * chan         - the channels struct
 * val_whole    - the whole part of the returned value
 * val_frac     - the fractional part of the returned value
 * mask         - the operation which will be performed
 * 
 * returns  IIO_VAL_INT and sets val to the enable value when mask is IIO_CHAN_INFO_ENABLE
 * returns   IIO_VAL_INT and sets the val to the channel number when mask is IIO_CHAN_INFO_RAW
 * returns   -EINVAL when the mask has unexpected value
 */
static int ad5592r_s_read_raw(struct iio_dev* indio_dev,
    struct iio_chan_spec const *chan,
    int *val_whole,
    int *val_frac,
    long mask) {
        switch (mask)
        {
        case IIO_CHAN_INFO_RAW:
            if (chan->channel) {
                *val_whole = 87;
            } else {
                *val_whole = 420;
            }
            return IIO_VAL_INT;
        
        default:
            return -EINVAL;
        }
}

/**
 * Probe function executed when the devicetree is parsed
 * parameter spi - the parent spi controller
 * returns 0 on success or negative number on error  
 */
static int ad5592r_s_probe(struct spi_device *spi) {
    struct iio_dev *indio_dev;

    indio_dev = devm_iio_device_alloc(&spi->dev, 0);
    if (!indio_dev) {
        return -ENOMEM; // Error: Not enogh memory!
    }


    indio_dev->name = "ad5592r_s";
    indio_dev->info = &ad5592r_s_info;
    indio_dev->channels = ad5592r_s_channels;
    indio_dev->num_channels = ARRAY_SIZE(ad5592r_s_channels);


    return devm_iio_device_register(&spi->dev, indio_dev);
}

module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Octavian Ionut CRACIUN");
MODULE_DESCRIPTION("AD5592R_S ADC Driver");
MODULE_LICENSE("GPL v2");
