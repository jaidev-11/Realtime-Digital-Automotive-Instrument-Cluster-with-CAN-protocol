typedef struct CAM2_msg
{
    unsigned int  id;
    unsigned char rtr;
    unsigned char dlc;
    unsigned int  byteA;
} CAN2;

// lcd
void lcd_data(unsigned char);
void lcd_init(void);
void lcd_cmd(unsigned char);
void lcd_string(char *);
void lcd_cgram(void);

// delay
void delay_ms(unsigned int);

// can
void can2_init(void);
void Config_vic_CAN2_RX(void);

// main
void light_change(unsigned char, unsigned char);
void values_change(int, int);
void config_vic_for_timer1(void);
void config_timer1_intr(void);
