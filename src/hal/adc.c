#include "adc.h"
#include <linux/spi/spidev.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

static int        s_fd   = -1;
static uint8_t    s_mode = SPI_MODE_0;     // try MODE_0 first; change to _3 if needed
static uint32_t   s_hz   = 1000000;
static double     s_vref = 3.3;            // MCP3208 VREF (tie to 3.3V)

bool adc_init(const char *spidev_path)
{
    s_fd = open(spidev_path, O_RDWR);
    if (s_fd < 0) { perror("open spidev"); return false; }

    uint8_t bpw = 8;
    if (ioctl(s_fd, SPI_IOC_WR_MODE, &s_mode) < 0) { perror("WR_MODE"); return false; }
    if (ioctl(s_fd, SPI_IOC_WR_BITS_PER_WORD, &bpw)  < 0) { perror("WR_BPW");  return false; }
    if (ioctl(s_fd, SPI_IOC_WR_MAX_SPEED_HZ, &s_hz)  < 0) { perror("WR_SPEED");return false; }

    return true;
}

void adc_cleanup(void)
{
    if (s_fd >= 0) { close(s_fd); s_fd = -1; }
}

static uint16_t mcp3208_read_raw(int ch)
{
    uint8_t tx[3] = {0}, rx[3] = {0};
    tx[0] = 0x06 | ((ch & 0x04) >> 2);      // start=1, single-ended=1, D2
    tx[1] = (uint8_t)((ch & 0x03) << 6);    // D1,D0 in bits 7..6
    tx[2] = 0x00;

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = s_hz,
        .delay_usecs = 0,
        .bits_per_word = 8,
        .cs_change = 0
    };

    if (ioctl(s_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        perror("SPI_IOC_MESSAGE");
        return 0xFFFF;
    }
    return (uint16_t)(((rx[1] & 0x0F) << 8) | rx[2]);
}

bool adc_read_ch(int ch, unsigned *raw_out, double *volts_out)
{
    if (s_fd < 0 || ch < 0 || ch > 7) return false;
    uint16_t raw = mcp3208_read_raw(ch);
    if (raw == 0xFFFF) return false;
    if (raw_out)   *raw_out   = raw;
    if (volts_out) *volts_out = (raw * s_vref) / 4095.0;
    return true;
}
