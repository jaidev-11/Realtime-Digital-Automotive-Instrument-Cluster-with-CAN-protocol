#include <lpc21xx.h>
#include "headerr.h"

extern unsigned int RECEIVER_FLAG;
extern CAN2 R1;
extern int timer_flag;

void can2_init(void)
{
    VPBDIV   = 1;           // PCLK = 60 MHz
    PINSEL1 |= 0x00014000;  // P0.23 -> RD2, P0.24 -> TD2
    C2MOD    = 0x1;         // reset mode
    C2BTR    = 0x001C001D;  // 125 Kbps
    AFMR     = 2;           // accept all messages
    C2MOD    = 0;           // release reset mode
}

// CAN2 RX ISR — minimal work: copy registers, set flag, release buffer
void CAN2_RX_HANDLER(void) __irq
{
    R1.id    = C2RID;
    R1.dlc   = ((C2RFS >> 16) & 0xF);
    R1.rtr   = ((C2RFS >> 30) & 1);
    R1.byteA = C2RDA;
    RECEIVER_FLAG = 1; // signal main loop
    C2CMR = 0x4;       // release RX buffer
    VICVectAddr = 0;
}

void Config_vic_CAN2_RX(void)
{
    C2IER        = 0x01;                          // enable RX interrupt
    VICIntSelect = 0;                             // all IRQs
    VICVectAddr0 = (unsigned int)CAN2_RX_HANDLER;
    VICVectCntl0 = 27 | (1 << 5);                // slot 0 -> CAN2 RX (IRQ 27)
    VICIntEnable = (1 << 27);
}

// Timer1 ISR — 80ms blink tick
void timer1_handler(void) __irq
{
    T1IR       = 1; // clear MR0 flag
    timer_flag = 1;
    VICVectAddr = 0;
}

void config_vic_for_timer1(void)
{
    VICVectAddr1 = (unsigned int)timer1_handler;
    VICVectCntl1 = 5 | (1 << 5); // slot 1 -> Timer1
    VICIntEnable = (1 << 5);
}

void config_timer1_intr(void)
{
    int arr[] = {15, 60, 30, 15, 15};
    unsigned int pclk = arr[VPBDIV] * 1000; // ticks per 1ms

    T1PC  = 0;
    T1PR  = pclk;  // prescaler for 1ms
    T1MR0 = 80;    // interrupt every 80ms
    T1MCR = 3;     // reset TC and interrupt on match
    T1TCR = 1;     // start Timer1
}
