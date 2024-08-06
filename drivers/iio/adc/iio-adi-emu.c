#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>

static const struct iio_info adi_emu_info = {

};

static int adi_emu_probe(struct spi_device *spi);

static struct spi_driver adi_emu_driver =
{
    .driver = {
        .name = "iio-adi-emu",
    },
    .probe = adi_emu_probe
};


static int adi_emu_probe(struct spi_device *spi) {
    struct iio_dev *indio_dev;
    indio_dev = devm_iio_device_alloc(&spi->dev, 0);
    if (!indio_dev) {
        return -ENOMEM; // Error: Not enogh memory!
    }

    indio_dev->name = "iio-adi-emu";
    indio_dev->info = &adi_emu_info;

    return devm_iio_device_register(&spi->dev, indio_dev);
}

module_spi_driver(adi_emu_driver);

MODULE_AUTHOR("Octavian Ionut CRACIUN");
MODULE_DESCRIPTION("IIO ADI Emulator Driver");
MODULE_LICENSE("GPL v2");
