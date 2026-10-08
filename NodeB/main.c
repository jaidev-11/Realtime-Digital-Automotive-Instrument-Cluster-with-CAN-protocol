#include <lpc21xx.h>
#include "headerr.h"

CAN2 R1;

#define LED1 (1 << 17) // Left Indicator LED
#define LED2 (1 << 18) // Headlight LED
#define LED3 (1 << 19) // Right Indicator LED

unsigned int  RECEIVER_FLAG;
unsigned char cur_sw = 0, prev_sw = 0, sw_stat = 0, event = 0;
unsigned char left_flag = 0, right_flag = 0;
int speed, temp;
int count1 = 0, count2 = 0;
int timer_flag;

int main()
{
    IODIR0 |= LED1 | LED2 | LED3; // set LED pins as output
    IOCLR0  = LED1 | LED2 | LED3; // all LEDs OFF initially
    delay_ms(100);
    IOSET0 |= LED1 | LED2 | LED3; // startup: all LEDs ON briefly

    lcd_init();
    can2_init();
    Config_vic_CAN2_RX();
    lcd_cgram();
    config_vic_for_timer1();
    config_timer1_intr();

    while (1)
    {
        // --- CAN frame received ---
        if (RECEIVER_FLAG == 1)
        {
            RECEIVER_FLAG = 0;

            if (R1.id == 0x10) // Event frame: switch state only
            {
                sw_stat = (R1.byteA & 0xFF);
                event   = 1;
            }

            if (R1.id == 0x50) // Periodic frame: speed + temp + switch state
            {
                sw_stat = ((R1.byteA >> 16) & 0xFF);
                speed   = (R1.byteA & 0xFF);
                temp    = ((R1.byteA >> 8) & 0xFF);
                event   = 1;
                values_change(speed, temp); // update LCD
            }

            // --- Process switch state change ---
            if ((event == 1) && (sw_stat != prev_sw))
            {
                cur_sw = (sw_stat ^ prev_sw); // detect changed bits
                light_change(cur_sw, sw_stat);
                prev_sw = sw_stat;
                event   = 0;
            }
        }

        // --- Timer flag: LED blink every 80ms ---
        if (timer_flag)
        {
            timer_flag = 0;

            // Left indicator blink
            if (sw_stat & (1 << 1))
            {
                if (left_flag == 0) { left_flag = 1; IOCLR0 = LED1; }
                else                { left_flag = 0; IOSET0 = LED1; }
            }
            else if (left_flag == 1)
            {
                IOSET0    = LED1; // force LED ON when indicator turned off
                left_flag = 0;
            }

            // Right indicator blink
            if (sw_stat & (1 << 2))
            {
                if (right_flag == 0) { right_flag = 1; IOCLR0 = LED3; }
                else                 { right_flag = 0; IOSET0 = LED3; }
            }
            else if (right_flag == 1)
            {
                IOSET0     = LED3;
                right_flag = 0;
            }
        }
    }
}

// Update speed and temperature on LCD
void values_change(int speed, int temp)
{
    lcd_cmd(0x80); // line 1, position 0 -> speed
    lcd_data((speed / 100) + 48);
    lcd_data(((speed / 10) % 10) + 48);
    lcd_data((speed % 10) + 48);
    lcd_string("Km/h");

    lcd_cmd(0x88); // line 1, position 8 -> temperature
    lcd_data((temp / 100) + 48);
    lcd_data(((temp / 10) % 10) + 48);
    lcd_data((temp % 10) + 48);
    lcd_data(4);   // custom char 4: degree symbol
    lcd_data('C');
}

// Update LEDs and LCD icons based on switch change
void light_change(unsigned char cur_sw, unsigned char sw_stat)
{
    if (cur_sw & (1 << 0)) // Headlight changed
    {
        if (sw_stat & (1 << 0))
        {
            IOCLR0 = LED2;      // LED ON (active low)
            lcd_cmd(0xC7);
            lcd_data(3);        // custom char 3: headlight icon
        }
        else
        {
            IOSET0 = LED2;      // LED OFF
            lcd_cmd(0xC7);
            lcd_data(' ');
        }
    }

    if (cur_sw & (1 << 1)) // Left indicator changed
    {
        if (sw_stat & (1 << 1))
        {
            lcd_cmd(0xC1);
            lcd_data(1); // custom char 1: left arrow
            lcd_data(0); // custom char 0: horizontal bar
        }
        else
        {
            lcd_cmd(0xC1);
            lcd_data(' ');
            lcd_data(' ');
        }
    }

    if (cur_sw & (1 << 2)) // Right indicator changed
    {
        if (sw_stat & (1 << 2))
        {
            lcd_cmd(0xCD);
            lcd_data(0); // custom char 0: horizontal bar
            lcd_data(2); // custom char 2: right arrow
        }
        else
        {
            lcd_cmd(0xCD);
            lcd_data(' ');
            lcd_data(' ');
        }
    }
}
