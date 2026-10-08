#include <lpc21xx.h>
#include "headerr.h"

void lcd_data(unsigned char data)
{
    // Send upper nibble
    IOCLR1 = 0x00FE0000;
    IOSET1 = ((data & 0xF0) << 16);
    IOCLR1 = 1 << 18; // RS=1 (data)
    IOSET1 = 1 << 17;
    IOSET1 = 1 << 19; // EN=1
    delay_ms(2);
    IOCLR1 = 1 << 19; // EN=0

    // Send lower nibble
    IOCLR1 = 0x00FE0000;
    IOSET1 = ((data & 0x0F) << 20);
    IOCLR1 = 1 << 18;
    IOSET1 = 1 << 17;
    IOSET1 = 1 << 19; // EN=1
    delay_ms(2);
    IOCLR1 = 1 << 19; // EN=0
}

void lcd_cmd(unsigned char cmd)
{
    // Send upper nibble
    IOCLR1 = 0x00FE0000;
    IOSET1 = (cmd & 0xF0) << 16;
    IOCLR1 = 1 << 18; // RS=0 (command)
    IOCLR1 = 1 << 17;
    IOSET1 = 1 << 19; // EN=1
    delay_ms(2);
    IOCLR1 = 1 << 19; // EN=0

    // Send lower nibble
    IOCLR1 = 0x00FE0000;
    IOSET1 = (cmd & 0x0F) << 20;
    IOCLR1 = 1 << 18;
    IOCLR1 = 1 << 17;
    IOSET1 = 1 << 19; // EN=1
    delay_ms(2);
    IOCLR1 = 1 << 19; // EN=0
}

void lcd_init(void)
{
    IODIR1 |= 0x00FE0000; // set PORT1 pins as output
    PINSEL2 = 0;
    lcd_cmd(0x02); // return home (4-bit mode init)
    lcd_cmd(0x28); // 4-bit, 2-line, 5x7 font
    lcd_cmd(0x0C); // display ON, cursor OFF
    lcd_cmd(0x01); // clear display
}

void lcd_string(char *p)
{
    while (*p)
    {
        lcd_data(*p++);
    }
}

void lcd_cgram(void)
{
    // Custom characters stored in CGRAM
    unsigned char a[8] = {0x00, 0x00, 0x00, 0x1F, 0x1F, 0x00, 0x00, 0x00}; // horizontal bar
    unsigned char b[8] = {0x00, 0x01, 0x03, 0x07, 0x07, 0x03, 0x01, 0x00}; // left indicator arrow
    unsigned char c[8] = {0x00, 0x10, 0x18, 0x1C, 0x1C, 0x18, 0x10, 0x00}; // right indicator arrow
    unsigned char d[8] = {0x00, 0x0E, 0x11, 0x11, 0x11, 0x1F, 0x00, 0x0E}; // headlight icon
    unsigned char e[8] = {0x1C, 0x14, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00}; // degree symbol
    int i;

    lcd_cmd(0x40);      // CGRAM address: char 0 (horizontal bar)
    for (i = 0; i < 8; i++) lcd_data(a[i]);

    lcd_cmd(0x40 + 8);  // CGRAM address: char 1 (left arrow)
    for (i = 0; i < 8; i++) lcd_data(b[i]);

    lcd_cmd(0x40 + 16); // CGRAM address: char 2 (right arrow)
    for (i = 0; i < 8; i++) lcd_data(c[i]);

    lcd_cmd(0x40 + 24); // CGRAM address: char 3 (headlight)
    for (i = 0; i < 8; i++) lcd_data(d[i]);

    lcd_cmd(0x40 + 32); // CGRAM address: char 4 (degree)
    for (i = 0; i < 8; i++) lcd_data(e[i]);
}
