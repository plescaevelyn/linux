#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>

static int ad5592r_s_probe(struct spi_device*);
static int ad5592r_s_read_raw(struct iio_dev*, struct iio_chan_spec const*, int*, int*, long);
static int ad5592r_s_write_raw(struct iio_dev*, struct iio_chan_spec const*, int, int, long);

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
 * Driver state
 */
struct ad5582r_s_state {
    bool enable;
    int channel0;
    int channel1;
    int channel2;
    int channel3;
    int channel4;
    int channel5;
    int channel6;
    int channel7;
};

/**
 * IIO_INFO structure
 */
static const struct iio_info ad5592r_s_info = {
    .read_raw = &ad5592r_s_read_raw,
    .write_raw = &ad5592r_s_write_raw,
};

/**
 *  Channels specification structure 
 */
static const struct iio_chan_spec ad5592r_s_channels[] = {
    {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 0,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 1,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 2,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 3,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 4,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 5,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 6,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .indexed = 1,
        .channel = 7,
    }
};

/**
 * Function to read the raw values of the adc5592r_s channels
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
        struct ad5582r_s_state *state = iio_priv(indio_dev);

        switch (mask) {
            case IIO_CHAN_INFO_ENABLE:
                *val_whole = state->enable;
                return IIO_VAL_INT;
            case IIO_CHAN_INFO_RAW:
                if (state->enable) {
                    switch (chan->channel) {
                        case 0:
                            *val_whole = state->channel0;
                            break;
                        case 1:
                            *val_whole = state->channel1;
                            break;
                        case 2:
                            *val_whole = state->channel2;
                            break;
                        case 3:
                            *val_whole = state->channel3;
                            break;
                        case 4:
                            *val_whole = state->channel4;
                            break;
                        case 5:
                            *val_whole = state->channel5;
                            break;
                        case 6:
                            *val_whole = state->channel6;
                            break;
                        case 7:
                            *val_whole = state->channel7;
                            break;     
                    }
                    return IIO_VAL_INT;
                } else {
                    return -EINVAL;
                }
            default:
                return -EINVAL;
        }
}

/**
 * Function to write a raw value to the ad5592r_s channels
 * indio_dev    - the created iio-adi-emu device
 * chan         - the channels struct
 * val          - the whole part of the raw value
 * val1         - the fractional part of the raw value
 * mask         - the operation which will be performed
 * 
 * return 0 when the mask is IIO_CHAN_INFO_ENABLED
 * return -EINVAL when the mask has unexpected value
 */
static int ad5592r_s_write_raw(struct iio_dev* indio_dev,
    struct iio_chan_spec const *chan,
    int val_whole,
    int val_frac,
    long mask) {
        struct ad5582r_s_state *state = iio_priv(indio_dev);

        switch (mask) {
            case IIO_CHAN_INFO_ENABLE:
                state->enable = val_whole;
                return 0;
            case IIO_CHAN_INFO_RAW:
                switch (chan->channel) {
                    case 0:
                        state->channel0 = val_whole;
                        break;
                    case 1:
                        state->channel1 = val_whole;
                        break;
                    case 2:
                        state->channel2 = val_whole;
                        break;
                    case 3:
                        state->channel3 = val_whole;
                        break;
                    case 4:
                        state->channel4 = val_whole;
                        break;
                    case 5:
                        state->channel5 = val_whole;
                        break;
                    case 6:
                        state->channel6 = val_whole;
                        break;
                    case 7:
                        state->channel7 = val_whole;
                        break;
                }
                return 0;
            default:
                // Error: Invalid value for mask
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
    struct ad5582r_s_state *state;

    indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*state));
    if (!indio_dev) {
        return -ENOMEM; // Error: Not enogh memory!
    }

    state = iio_priv(indio_dev);
    state->enable = 0;
    state->channel0 = 0;
    state->channel1 = 0;
    state->channel2 = 0;
    state->channel3 = 0;
    state->channel4 = 0;
    state->channel5 = 0;
    state->channel6 = 0;
    state->channel7 = 0;

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
