#include <lpc21xx.h>
#include "header.h"

void adc_init(void)
{
    PINSEL1 |= 0x15400000; // P0.27 P0.28 P0.29 P0.30
    ADCR = 0x00200400;     // channels not selected, burst mode off, 10-bit, PDN=1
}

unsigned short int adc_read(unsigned short int ch_num)
{
    unsigned short int result = 0;
    ADCR |= (1 << ch_num); // select ADC channel
    ADCR |= (1 << 24);     // start ADC conversion
    while (((ADDR >> 31) & 1) == 0); // wait for conversion complete
    ADCR ^= (1 << 24);     // stop ADC
    ADCR ^= (1 << ch_num); // deselect channel
    result = ((ADDR >> 6) & 0x3FF); // extract 10-bit result
    return result;
}
