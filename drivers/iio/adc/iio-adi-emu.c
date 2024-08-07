#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>

static int adi_emu_probe(struct spi_device*);
static int adi_emu_read_raw(struct iio_dev*, struct iio_chan_spec const*, int*, int*, long);
static int adi_emu_write_raw(struct iio_dev*, struct iio_chan_spec const*, int, int, long);

/**
 * State structure
 */
struct adi_emu_state {
    bool enable;
};

/**
 * IIO_INFO structure
 */
static const struct iio_info adi_emu_info = {
    .read_raw = &adi_emu_read_raw,
    .write_raw = &adi_emu_write_raw,
};


/**
 * Driver structure
 */
static struct spi_driver adi_emu_driver =
{
    .driver = {
        .name = "iio-adi-emu",
    },
    .probe = adi_emu_probe
};

/**
 *  Channels specification structure 
 */
static const struct iio_chan_spec adi_emu_channels[] = {
    {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .output = 0,
        .indexed = 1,
        .channel = 0,
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .output = 0,
        .indexed = 1,
        .channel = 1,
    }
};

/**
 * Probe function executed when the devicetree is parsed
 * parameter spi - the parent spi controller
 * returns 0 on success or negative number on error  
 */
static int adi_emu_probe(struct spi_device *spi) {
    struct iio_dev *indio_dev;
    struct adi_emu_state *state;

    indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*state));
    if (!indio_dev) {
        return -ENOMEM; // Error: Not enogh memory!
    }

    state = iio_priv(indio_dev);
    state->enable = false;

    indio_dev->name = "iio-adi-emu";
    indio_dev->info = &adi_emu_info;
    indio_dev->channels = adi_emu_channels;
    indio_dev->num_channels = ARRAY_SIZE(adi_emu_channels);


    return devm_iio_device_register(&spi->dev, indio_dev);
}

/**
 * Function to read the raw value of the adi_emu channels
 * indio_dev    - the created iio-adi-emu device
 * chan         - the channels struct
 * val          - the whole part of the returned value
 * val1         - the fractional part of the returned value
 * mask         - the operation which will be performed
 * 
 * returns  IIO_VAL_INT and sets val to the enable value when mask is IIO_CHAN_INFO_ENABLE
 * returns   IIO_VAL_INT and sets the val to the channel number when mask is IIO_CHAN_INFO_RAW
 * returns   -EINVAL when the mask has unexpected value
 */
static int adi_emu_read_raw(struct iio_dev *indio_dev,
    struct iio_chan_spec const *chan,
    int *val,
    int*val2,
    long mask) {
        struct adi_emu_state *state = iio_priv(indio_dev);

        switch (mask) {
            case IIO_CHAN_INFO_ENABLE:
                *val = state->enable;
                return IIO_VAL_INT;
            case IIO_CHAN_INFO_RAW:
                *val = chan->channel;
                return IIO_VAL_INT;
            default:
                // Error: Invalid value for mask
                return -EINVAL;
        }
}

/**
 * Function to write a raw value to the adi_emu channels
 * indio_dev    - the created iio-adi-emu device
 * chan         - the channels struct
 * val          - the whole part of the raw value
 * val1         - the fractional part of the raw value
 * mask         - the operation which will be performed
 * 
 * return 0 when the mask is IIO_CHAN_INFO_ENABLED
 * return -EINVAL when the mask has unexpected value
 */
static int adi_emu_write_raw(struct iio_dev *indio_dev,
    struct iio_chan_spec const *chan,
    int val,
    int val2,
    long mask) {
        struct adi_emu_state *state = iio_priv(indio_dev);
        
        switch (mask) {
            case IIO_CHAN_INFO_ENABLE:
                state->enable = val;
                return 0;
            default:
                // Error: Invalid value for mask
                return -EINVAL;
        }
}

module_spi_driver(adi_emu_driver);

MODULE_AUTHOR("Octavian Ionut CRACIUN");
MODULE_DESCRIPTION("IIO ADI Emulator Driver");
MODULE_LICENSE("GPL v2");
