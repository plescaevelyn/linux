#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>
#include <linux/bitfield.h>
#include <asm/unaligned.h>

#define IIO_ADC_EMU_READ_MASK       BIT(7)
#define IIO_ADC_EMU_VALUE_MASK      GENMASK(7,0)
#define IIO_ADC_ADDR_READ_MASK      GENMASK(6,0)
#define IIO_ADC_ADDR_WRITE_MASK     GENMASK(14,8)

/**
 * State structure
 */
struct adi_emu_state {
    bool enable;
    struct spi_device *spi;
    int channel0;
    int channel1;
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
    }};
 

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
    int *val2,
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
            case IIO_CHAN_INFO_RAW:
                if (chan->channel) {
                    state->channel1 = val;
                } else {
                    state->channel0 = val;
                }
                return IIO_VAL_INT;
            case IIO_CHAN_INFO_ENABLE:
                state->enable = val;
                return IIO_VAL_INT;
            default:
                // Error: Invalid value for mask
                return -EINVAL;
        }
}

/**
 * Function to read one byte from adi_emu using SPI
 */
static int adi_emu_spi_read(struct adi_emu_state *state,
    u8 reg_addr,
    u8 *read_val) {
        u8 tx, rx;
        struct spi_transfer xfer[] = {
            {
                .rx_buf = NULL,
                .tx_buf = &tx,
                .len = 1,
            }, {
                .rx_buf = &rx,
                .tx_buf = NULL,
                .len = 1,
            }
        };

        tx = IIO_ADC_EMU_READ_MASK | FIELD_PREP(IIO_ADC_ADDR_READ_MASK, reg_addr);
        int ret = spi_sync_transfer(state->spi, xfer, 2);
        if (ret) {
            dev_err(&state->spi->dev, "SPI sync transfer failed on read");
            return ret;
        }

        *read_val = rx;
        return 0;
}

/**
 * Function to write one byte to adi_emu using SPI
 */
static int adi_emu_spi_write(struct adi_emu_state *state,
    u8 reg_addr,
    u8 write_val) {
        u16 tx = 0, msg = 0;
        struct spi_transfer xfer = {
            .rx_buf = NULL,
            .tx_buf = &tx,
            .len = 2,
        };

        msg = FIELD_PREP(IIO_ADC_ADDR_WRITE_MASK, reg_addr) |
            FIELD_PREP(IIO_ADC_EMU_VALUE_MASK, write_val);
        
        dev_info(&state->spi->dev, "msg = 0x%X", msg);

        put_unaligned_be16(msg, &tx);
        dev_info(&state->spi->dev, "tx = 0x%X", tx);

        int ret = spi_sync_transfer(state->spi, &xfer, 1);
        if (ret) {
            dev_err(&state->spi->dev, "SPI sync transfer failed on write");
            return ret;
        }

        return 0;
}

static int adi_emu_debugfx(struct iio_dev* indio_dev,
    unsigned reg_addr,
    unsigned write_val,
    unsigned *read_val) {
        struct adi_emu_state *state = iio_priv(indio_dev);

        if (read_val) {
            return adi_emu_spi_read(state, reg_addr, (u8 *)read_val);
        } else {
            return adi_emu_spi_write(state, reg_addr, write_val);
        }
}

/**
 * IIO_INFO structure
 */
static const struct iio_info adi_emu_info = {
    .read_raw = &adi_emu_read_raw,
    .write_raw = &adi_emu_write_raw,
    .debugfs_reg_access = &adi_emu_debugfx,
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
    state->spi = spi;

    indio_dev->name = "iio-adi-emu";
    indio_dev->info = &adi_emu_info;
    indio_dev->channels = adi_emu_channels;
    indio_dev->num_channels = ARRAY_SIZE(adi_emu_channels);


    return devm_iio_device_register(&spi->dev, indio_dev);
}

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

module_spi_driver(adi_emu_driver);

MODULE_AUTHOR("Octavian Ionut CRACIUN");
MODULE_DESCRIPTION("IIO ADI Emulator Driver");
MODULE_LICENSE("GPL v2");
