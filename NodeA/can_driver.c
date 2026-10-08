#include <LPC21xx.H>
#include "header.h"

#define TCS ((C2GSR >> 3) & 1) // Transmission Complete Status

void can2_init(void)
{
    VPBDIV   = 1;            // PCLK = 60 MHz
    PINSEL1 |= 0x00014000;   // P0.23 -> RD2, P0.24 -> TD2
    C2MOD    = 1;            // enter reset mode
    C2BTR    = 0x001C001D;   // 125 Kbps baud rate
    AFMR     = 2;            // accept all incoming messages
    C2MOD    = 0;            // release reset mode
}

void can2_tx(CAN2 v)
{
    C2TID1 = v.id;
    C2TFI1 = ((v.dlc << 16) & (0x7 << 16)); // set DLC, RTR=0, FF=0

    if (v.rtr == 0)
    {
        C2TDA1 = v.byteA;
    }

    C2CMR = 0x21;        // select TX buffer 1 and start transmission
    while (TCS == 0);    // wait for transmission complete
}
