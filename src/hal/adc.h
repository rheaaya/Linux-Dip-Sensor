// adc.h - Hardware abstraction for MCP3208 ADC reading
// Provides interface to read light sensor via SPI

#ifndef HAL_ADC_H
#define HAL_ADC_H

#include <stdbool.h>

#define ADC_VREF 3.3           // MCP3208 reference voltage
#define ADC_MAX_RAW 4095       // 12-bit resolution
#define ADC_SPI_SPEED_HZ 1000000
#define ADC_SPI_MODE SPI_MODE_0

bool  adc_init(const char *spidev_path);    // e.g., "/dev/spidev0.0"
void  adc_cleanup(void);

// Read single-ended channel (0..7). Returns raw 0..4095 and volts (0..Vref)
bool  adc_read_ch(int ch, unsigned *raw_out, double *volts_out);

#endif
