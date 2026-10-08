#include <lpc21xx.h>
#include "header.h"

extern int Head_flag;
extern int Right_flag;
extern int Left_flag;
extern int speed_tick;

void config_vic_for_extint(void)
{
    PINSEL0 |= 0xA0000000; // EINT1 & EINT2
    PINSEL1 |= 0x00000001; // EINT0
    EXTMODE  = 0x7;        // edge triggered
    EXTPOLAR = 0x0;        // active low

    VICIntSelect = 0;      // all IRQs

    VICVectCntl0 = 14 | (1 << 5); // slot 0 -> EINT0 (Headlight)
    VICVectAddr0 = (unsigned int)EINT0_Handler;

    VICVectCntl1 = 15 | (1 << 5); // slot 1 -> EINT1 (Left Indicator)
    VICVectAddr1 = (unsigned int)EINT1_Handler;

    VICVectCntl2 = 16 | (1 << 5); // slot 2 -> EINT2 (Right Indicator)
    VICVectAddr2 = (unsigned int)EINT2_Handler;

    VICIntEnable |= (1 << 14) | (1 << 15) | (1 << 16); // enable EINT0/1/2
}

void EINT0_Handler(void) __irq // Headlight ISR
{
    Head_flag = 1;
    EXTINT = 1;
    VICVectAddr = 0;
}

void EINT1_Handler(void) __irq // Left Indicator ISR
{
    Left_flag = 1;
    EXTINT = 2;
    VICVectAddr = 0;
}

void EINT2_Handler(void) __irq // Right Indicator ISR
{
    Right_flag = 1;
    EXTINT = 4;
    VICVectAddr = 0;
}

void config_vic_for_timer1(void)
{
    VICVectCntl4 = 5 | (1 << 5);           // slot 4 -> Timer1
    VICVectAddr4 = (unsigned int)Timer1_Handler;
    VICIntEnable |= (1 << 5);              // enable Timer1 interrupt
}

void config_timer1_intr(void)
{
    int arr[] = {15, 60, 30, 15, 15};
    unsigned int pclk = arr[VPBDIV % 4] * 1000; // ticks per 1ms

    T1PC  = 0;
    T1PR  = pclk - 1; // prescaler for 1ms resolution
    T1MR0 = 10;       // interrupt every 10ms
    T1MCR = 3;        // reset TC and interrupt on match
    T1TCR = 1;        // start Timer1
}

void Timer1_Handler(void) __irq // 10ms tick ISR
{
    T1IR      = 1; // clear MR0 interrupt flag
    speed_tick = 1;
    VICVectAddr = 0;
}
