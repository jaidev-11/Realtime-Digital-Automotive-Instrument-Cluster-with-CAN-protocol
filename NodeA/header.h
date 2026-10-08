typedef struct CAN2_MSG
{
    unsigned char rtr;
    unsigned int id;
    unsigned int byteA;
    unsigned char dlc;
} CAN2;

// can
void can2_init(void);
void can2_tx(CAN2);

// adc
void adc_init(void);
unsigned short int adc_read(unsigned short int);

// interrupts
void config_vic_for_extint(void);
void EINT0_Handler(void) __irq;
void EINT1_Handler(void) __irq;
void EINT2_Handler(void) __irq;
void config_vic_for_timer1(void);
void config_timer1_intr(void);
void Timer1_Handler(void) __irq;
