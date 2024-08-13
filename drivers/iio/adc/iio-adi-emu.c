#include <asm/unaligned.h>

#include <linux/bitfield.h>
#include <linux/module.h>

#include <linux/spi/spi.h>

#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>

#define IIO_ADC_EMU_READ_MASK           BIT(7)
#define IIO_ADC_EMU_VALUE_MASK          GENMASK(7,0)
#define IIO_ADC_ADDR_READ_MASK          GENMASK(6,0)
#define IIO_ADC_ADDR_WRITE_MASK         GENMASK(14,8)

#define IIO_ADC_ADDR_REG_CNVST          0x3
#define IIO_ADC_ADDR_REG_ENABLE         0x2
#define IIO_ADC_ADDR_REG_CHAN_HIGH(x)   0x4 + x * 2
#define IIO_ADC_ADDR_REG_CHAN_LOW(x)    0x5 + x * 2
#define IIO_ADC_ADDR_REG_CHIP_ID        0x0

#define IIO_ADC_START_CNVST_VAL         BIT(0)
#define IIO_ADC_ENABLE_VAL              0x0

struct adi_emu_state {
    bool enable;
    struct spi_device *spi;
    int channel0;
    int channel1;
};

static const struct iio_chan_spec adi_emu_channels[] = {
    {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .output = 0,
        .indexed = 1,
        .channel = 0,
        .scan_index = 0,
        .scan_type = {
            .sign = 'u',
            .realbits = 12,
            .storagebits = 16,
        }
    }, {
        .type = IIO_VOLTAGE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
        .output = 0,
        .indexed = 1,
        .channel = 1,
        .scan_index = 1,
        .scan_type = {
            .sign = 'u',
            .realbits = 12,
            .storagebits = 16,
        }
    }};

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

static int adi_emu_read_channel(struct iio_dev *indio_dev, struct iio_chan_spec *chan, int *val) {
    struct adi_emu_state *state = iio_priv(indio_dev);


    int ret = adi_emu_spi_write(state, IIO_ADC_ADDR_REG_CNVST, IIO_ADC_START_CNVST_VAL);
    if (ret) {
        dev_err(&state->spi->dev, "Failed CNVST reg write");
        return ret;
    }

    u8 valHigh, valLow = 0;
    ret = adi_emu_spi_read(state, IIO_ADC_ADDR_REG_CHAN_HIGH(chan->channel), &valHigh);
    if (ret) {
        dev_err(&state->spi->dev, "Failed READ channel %d high reg", chan->channel);
        return ret;
    }

    ret = adi_emu_spi_read(state, IIO_ADC_ADDR_REG_CHAN_LOW(chan->channel), &valLow);
    if (ret) {
        dev_err(&state->spi->dev, "Failed READ channel %d low reg", chan->channel);
        return ret;
    }

    *val = (valHigh << 8) | valLow;
    return 0;
}

static int adi_emu_read_raw(struct iio_dev *indio_dev,
    struct iio_chan_spec const *chan,
    int *val,
    int *val2,
    long mask) {
        struct adi_emu_state *state = iio_priv(indio_dev);
        int ret = 0;

        switch (mask) {
            case IIO_CHAN_INFO_ENABLE:
                *val = state->enable;
                return IIO_VAL_INT;
            case IIO_CHAN_INFO_RAW:
                ret = adi_emu_read_channel(indio_dev, chan, val);
                if (ret) {
                    dev_err(&state->spi->dev, "Failed READ raw");
                    return ret;
                }
                return IIO_VAL_INT;
            default:
                return -EINVAL;
        }
}

static int adi_emu_write_raw(struct iio_dev *indio_dev,
    struct iio_chan_spec const *chan,
    int val,
    int val2,
    long mask) {
        struct adi_emu_state *state = iio_priv(indio_dev);

        switch (mask) {
            case IIO_CHAN_INFO_ENABLE:
                state->enable = val;
                return IIO_VAL_INT;
            default:
                return -EINVAL;
        }
}

static irqreturn_t adi_emu_trigger_handler(int irq, void *p)
{
    struct iio_poll_func *pf = p;
    struct iio_dev *indio_dev = pf->indio_dev;
    struct adi_emu_state *state = iio_priv(indio_dev);
    u16 buf[2];
    u8 valHigh, valLow;
    int i = 0;
    int bit = 0;
    int ret = 0;

    ret = adi_emu_spi_write(state, IIO_ADC_ADDR_REG_CNVST, IIO_ADC_START_CNVST_VAL);
    if (ret) {
        dev_err(&state->spi->dev, "Failed CNVST reg write in handler");
        return IRQ_HANDLED;
    }

    for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->num_channels) {
        ret = adi_emu_spi_read(state, IIO_ADC_ADDR_REG_CHAN_HIGH(bit), &valHigh);
        if (ret) {
            dev_err(&state->spi->dev, "Failed READ channel %d high reg in handler", bit);
            return IRQ_HANDLED;
        }

        ret = adi_emu_spi_read(state, IIO_ADC_ADDR_REG_CHAN_LOW(bit), &valLow);
        if (ret) {
            dev_err(&state->spi->dev, "Failed READ channel %d low reg in handler", bit);
            return IRQ_HANDLED;
        }

        buf[i] = (valHigh << 8) | valLow;
        i++;
    }

    ret = iio_push_to_buffers(indio_dev, buf);
    if (ret) {
        dev_err(&state->spi->dev, "Failed PUSH to buffer in handler");
        return IRQ_HANDLED;
    }

    iio_trigger_notify_done(indio_dev->trig);
    return IRQ_HANDLED;
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

static const struct iio_info adi_emu_info = {
    .read_raw = &adi_emu_read_raw,
    .write_raw = &adi_emu_write_raw,
    .debugfs_reg_access = &adi_emu_debugfx,
};

static int adi_emu_probe(struct spi_device *spi) {
    struct iio_dev *indio_dev;
    struct adi_emu_state *state;

    indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*state));
    if (!indio_dev) {
        return -ENOMEM;
    }

    state = iio_priv(indio_dev);
    state->enable = false;
    state->spi = spi;

    indio_dev->name = "iio-adi-emu";
    indio_dev->info = &adi_emu_info;
    indio_dev->channels = adi_emu_channels;
    indio_dev->num_channels = ARRAY_SIZE(adi_emu_channels);

    int ret = adi_emu_spi_write(state, IIO_ADC_ADDR_REG_ENABLE, IIO_ADC_ENABLE_VAL);
    if (ret) {
        dev_err(&state->spi->dev, "Failed ENABLE");
        return ret;
    }
    state->enable = true;

    ret = devm_iio_triggered_buffer_setup_ext(&spi->dev,
        indio_dev,
        NULL,
        adi_emu_trigger_handler,
        IIO_BUFFER_DIRECTION_IN,
        NULL,
        NULL);

    return devm_iio_device_register(&spi->dev, indio_dev);
}

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
