#include <lpc21xx.h>
#include "header.h"

#define Event_ID  0x10
#define Period_ID 0x50

int Head_flag  = 0;
int Right_flag = 0;
int Left_flag  = 0;
int speed_tick = 0;

CAN2 period, event;

int main()
{
    int avg_tick = 0, can_tick = 0, temp_tick = 0;
    unsigned short int speed = 0, temp = 0, vout;
    unsigned int add = 0, avg_speed, adc_val;
    char event_ff = 0;
    unsigned char switch_stat = 0, prev_stat = 0;

    period.id  = Period_ID;
    event.id   = Event_ID;
    period.dlc = 3;
    event.dlc  = 1;
    event.rtr  = 0;
    period.rtr = 0;

    can2_init();
    adc_init();
    config_vic_for_extint();
    config_vic_for_timer1();
    config_timer1_intr();

    while (1)
    {
        // --- Headlight toggle ---
        if (Head_flag)
        {
            Head_flag = 0;
            switch_stat ^= (1 << 0); // bit 0 -> headlight
            event_ff = 1;
        }

        // --- Left Indicator toggle (mutually exclusive with Right) ---
        if (Left_flag)
        {
            Left_flag = 0;
            switch_stat ^= (1 << 1);  // toggle left
            switch_stat &= ~(1 << 2); // force-clear right
            event_ff = 1;
        }

        // --- Right Indicator toggle (mutually exclusive with Left) ---
        if (Right_flag)
        {
            Right_flag = 0;
            switch_stat ^= (1 << 2);  // toggle right
            switch_stat &= ~(1 << 1); // force-clear left
            event_ff = 1;
        }

        // --- Send event frame only on state change ---
        if ((event_ff == 1) && (switch_stat != prev_stat))
        {
            event.byteA = switch_stat;
            can2_tx(event);
            prev_stat = switch_stat;
            event_ff  = 0;
        }

        // --- 10ms tick: ADC sampling and counter increments ---
        if (speed_tick)
        {
            speed_tick = 0;
            add += adc_read(2); // potentiometer on ADC Ch2
            avg_tick++;
            can_tick++;
            temp_tick++;
        }

        // --- Average speed over 10 samples (100ms window) ---
        if (avg_tick >= 10)
        {
            avg_speed = add / 10;
            add       = 0;
            avg_tick  = 0;
            speed     = ((avg_speed * 120) / 1023); // map to 0-120 Km/h
        }

        // --- Periodic CAN TX every 150ms ---
        if (can_tick >= 15)
        {
            period.byteA = ((speed & 0xFF) | ((temp & 0xFF) << 8) | ((switch_stat & 0xFF) << 16));
            can2_tx(period);
            can_tick = 0;
        }

        // --- Temperature read every 200ms ---
        if (temp_tick >= 20)
        {
            adc_val   = adc_read(1); // LM35 on ADC Ch1
            vout      = (adc_val * 3.3) / 1023;    // convert to voltage
            temp      = (vout - 0.5) / 0.01;        // LM35: 10mV/C, offset 500mV
            temp_tick = 0;
        }
    }
}
