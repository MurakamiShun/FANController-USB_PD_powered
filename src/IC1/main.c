/*
 * File:   main.c
 * Author: MurakamiShun
 *
 * Created on 2021/07/01, 0:56
 */
#pragma config CLKOUTEN = OFF, PLLEN = ON, FOSC = INTOSC
#pragma config LVP = OFF, MCLRE = OFF
#pragma config CP = ON, CPD = ON
#pragma config BOREN = ON, STVREN = OFF, IESO = OFF, FCMEN = OFF, BORV = LO
#pragma config WDTE = OFF, PWRTE = OFF,

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define _XTAL_FREQ 32000000L

void init_i2c(void){
    SSP1CON1bits.SSPEN = 1;
    SSP1CON1bits.SSPM = 0b1000; // i2c master
    SSP1CON2 = 0x00;
    SSP1CON3 = 0x00;
    SSP1STAT = 0x00;
    // _XTAL_FREQ/(CLK * 4) - 1
    SSP1ADD = 0x13; // 400kbps
    SSP1STATbits.SMP = 0;
    //SSP1ADD = 0x4F; // 100kbps
    //SSP1STATbits.SMP = 1;
}

void send_i2c(uint8_t data){
    SSP1IF = 0;
    SSP1BUF = data;
    while(!SSP1IF);
    SSP1IF = 0;
}

void begin_i2c_transmission(uint8_t address){
    // start condition
    SSP1CON2bits.SEN = 1;
    while(SSP1CON2bits.SEN);
    // send address
    send_i2c(address);
}

void end_i2c_transmission(void){
    // stop condition
    SSP1CON2bits.PEN = 1;
    while(SSP1CON2bits.PEN);
}

void send_lcd_command(uint8_t command){
    begin_i2c_transmission(0x78);
    send_i2c(0x00);
    send_i2c(command);
    end_i2c_transmission();
    __delay_us(50);
}

void send_lcd_data(uint8_t data){
    begin_i2c_transmission(0x78);
    send_i2c(0x40);
    send_i2c(data);
    end_i2c_transmission();
    __delay_us(50);
}

void init_lcd(void){
    __delay_ms(100);
    begin_i2c_transmission(0x78);
    send_i2c(0x00); // control
    send_i2c(0xAE); // display off
    
    send_i2c(0xA8); // set multiplex ratio
    send_i2c(0b00111111); // 64MUX
    
    send_i2c(0xD3); // set display offset
    send_i2c(0x00); // 0
    
    send_i2c(0x40 | 0x00); // set display start line to 0x00
    send_i2c(0xA0); // set segment re-map
    send_i2c(0xC0); // set COM output scan direction
    
    send_i2c(0xDA); // set COM pins hardware configurationo
    send_i2c(0b00010010);
    
    send_i2c(0x81); // set contrast
    send_i2c(0xFF); // max
    
    send_i2c(0xA4); // disable entire display on
    send_i2c(0xA6); // set normal display
    
    send_i2c(0xD5); // set display clock frequency/divide
    send_i2c(0b10000000);
    
    send_i2c(0x8D); // set charge pump
    send_i2c(0x14);
    
    send_i2c(0x2E); // stop scrolling
    send_i2c(0x20); // set memory addressing mode
    send_i2c(0x00); // horizontal
    
    send_i2c(0x21); // set column address
    send_i2c(0x00); // start column address
    send_i2c(0x7F); // stop column address
    
    send_i2c(0x22); // set page address
    send_i2c(0x00); // start page address
    send_i2c(0x07); // stop page address
    
    send_i2c(0xAF); // display on
    end_i2c_transmission();
    
    for(uint8_t page = 0; page < 8; ++page){
        begin_i2c_transmission(0x78);
        send_i2c(0x00);
        send_i2c(0xB0 | page); // set page
        send_i2c(0x21); // set col
        send_i2c(0x00); // begin col
        send_i2c(0x7F); // end col
        end_i2c_transmission();
        
        for(uint8_t col = 0; col < 16; ++col){
            begin_i2c_transmission(0x78);
            send_i2c(0x40);
            for(uint8_t b = 0; b < 8; ++b){
                send_i2c(0x00);
            }   
            end_i2c_transmission();
        }
    }
}


uint16_t TMR1_reset_count = 0;
// [ms]
uint16_t interval_RA[2][2] = {{0, 0}, {0, 0}};

void init_rot_freq(void){
    GIE = 1; // enable interrupt
    PEIE = 1;
    T1CON = 0x00;  // focs
    T1CONbits.T1CKPS = 0b11; // timer1 8MHz/8
    TMR1H = 0;
    TMR1L = 0;
    T1CONbits.TMR1ON = 1; // enable timer1
    // count up to 1000;
    CCPR1H = 0x03;
    CCPR1L = 0xe8;
    CCP1CONbits.CCP1M = 0b1011; // compare and reset TMR1
    CCP1IE = 1; // enable timer1 interrupt
    CCP1IF = 0;
    
    IOCANbits.IOCAN4 = 1; // enable RA4 interrupt
    IOCAFbits.IOCAF4 = 0;
    IOCANbits.IOCAN5 = 1; // enable RA4 interrupt
    IOCAFbits.IOCAF5 = 0;
    INTCONbits.IOCIE = 1; // enable gpio interrupt
    OPTION_REGbits.INTEDG = 1; // gpio interrupt is high-to-low edge
}

void init(void){
    OSCCONbits.IRCF = 0b1110; // 8MHz
    OSCCONbits.SPLLEN = 1; // 4xPLL
    OSCCONbits.SCS = 0b00;
    ANSELA = 0x00; // RA are all degital
    TRISA = 0b00110110;  // RA1 RA2 RA4 RA5 are input
    WPUA = 0b00110110;  // pull up
    nWPUEN = 0; // enable pull up
    PORTA = 0x00;
    
    init_i2c();
    init_lcd();
    init_rot_freq();
}

void interrupt TMR1_inter(void){
    if(CCP1IF){ // timer1 overflow interrupt
        // 1 [ms]
        TMR1_reset_count += 1;
        CCP1IF = 0;
    }
    if(IOCAFbits.IOCAF4){ // RA4 edge-down interrupt
        interval_RA[0][0] = interval_RA[0][1];
        interval_RA[0][1] = TMR1_reset_count;
        IOCAFbits.IOCAF4 = 0;
    }
    else if(IOCAFbits.IOCAF5){ // RA5 edge-down interrupt
        interval_RA[1][0] = interval_RA[1][1];
        interval_RA[1][1] = TMR1_reset_count;
        IOCAFbits.IOCAF5 = 0;
    }
}

__EEPROM_DATA(
    // 0
    0b01111110,
    0b10000001,
    0b10000001,
    0b01111110,
    // 1
    0b00000000,
    0b10000010,
    0b11111111,
    0b10000000
);__EEPROM_DATA(
    // 2
    0b11000010,
    0b10100001,
    0b10010001,
    0b10001110,
    // 3
    0b01000010,
    0b10001001,
    0b10001001,
    0b01110110
);__EEPROM_DATA(
    // 4
    0b00111000,
    0b00100110,
    0b11111111,
    0b00100000,
    // 5
    0b10001111,
    0b10001001,
    0b10001001,
    0b01110001
);__EEPROM_DATA(
    // 6
    0b01111110,
    0b10001001,
    0b10001001,
    0b01110010,
    // 7
    0b00000001,
    0b11100001,
    0b00011001,
    0b00000111
);__EEPROM_DATA(
    // 8
    0b01110110,
    0b10001001,
    0b10001001,
    0b01110110,
    // 9
    0b01000110,
    0b10001001,
    0b10001001,
    0b01111110
);__EEPROM_DATA(
    // A
    0b11111100,
    0b00100011,
    0b00100011,
    0b11111100,
    // B
    0b11111111,
    0b10001001,
    0b10001001,
    0b01110110
);__EEPROM_DATA(
    0x00, // padding
    // r
    0b11111000,
    0b00010000,
    0b00001000,
    0b00001000,
    0x00, // padding
    // p
    0b11111000,
    0b00101000
);__EEPROM_DATA(
    0b00101000,
    0b00010000,
    0x00, // padding
    // m (5byte)
    0b11111000,
    0b00001000,
    0b11110000,
    0b00001000,
    0b11110000
);

uint8_t read_x2font(uint8_t addr, uint8_t page){
    uint8_t font = eeprom_read(addr);
    if(page == 0){
        font = font & 0x0F;
    }
    else{
        font = (font & 0xF0) >> 4;
    }
    font = font & 0x01
        | ((font << 1) & 0x04)
        | ((font << 2) & 0x10)
        | ((font << 3) & 0x40);
    font = font | (font << 1);
}

void lcd_puts_rpm(uint16_t rpm, uint8_t col){
    char str[5] = {0};
    sprintf(str, "%4u", rpm);
    for(uint8_t page = 0; page < 2; ++page){
        begin_i2c_transmission(0x78);
        send_i2c(0x00);
        send_i2c(0xB0 | (page+1+col*4)); // set page
        send_i2c(0x21); // set col
        send_i2c(0x00); // begin col
        send_i2c(64+4+16); // end col
        end_i2c_transmission();
        
        // "A:" or "B:"
        begin_i2c_transmission(0x78);
        send_i2c(0x40);
        send_i2c(0x00);send_i2c(0x00);
        for(uint8_t b = 0; b < 4; ++b){
            uint8_t font = read_x2font(col * 4 + b + 0x28, page);

            send_i2c(font);
            send_i2c(font);
        }
        send_i2c(0x00);send_i2c(0x00);
        send_i2c(0x30);send_i2c(0x30);
        send_i2c(0x00);send_i2c(0x00);
        end_i2c_transmission();
        
        // rpm integers
        for(uint8_t chara = 0; chara < 4; ++chara){
            begin_i2c_transmission(0x78);
            send_i2c(0x40);
            for(uint8_t b = 0; b < 4; ++b){
                if('0' <= str[chara] && str[chara]  <= '9'){
                    uint8_t font = read_x2font((str[chara] - '0')*4+b, page);
                    
                    send_i2c(font);
                    send_i2c(font);
                }
                else{
                    send_i2c(0x00);
                    send_i2c(0x00);
                }
            }
            send_i2c(0x00);
            end_i2c_transmission();
        }
        // "rpm"
        for(uint8_t chara = 0; chara < 4; ++chara){
            begin_i2c_transmission(0x78);
            send_i2c(0x40);
            for(uint8_t b = 0; b < 4; ++b){
                uint8_t font = read_x2font((chara)*4+b+0x30, page);

                send_i2c(font);
                send_i2c(font);
            }
            end_i2c_transmission();
        }
    }
}

void main(void) {
    init();
    
    while(1){
        const uint8_t lcd_command[2] = { 0x01, 0xc0 | 0x40}; // clear, goto 2nd line
        for(uint8_t pin = 0; pin < 2; ++pin){
            char str[5] = {0};
            send_lcd_command(lcd_command[pin]);
            uint16_t rpm = 0;
            if(interval_RA[pin][0] <= interval_RA[pin][1]){
                rpm = interval_RA[pin][1] - interval_RA[pin][0];
            }
            else{
                rpm = UINT16_MAX - interval_RA[pin][0] + interval_RA[pin][1];
            }
            rpm = 30000 / rpm; // 2pulse per rorate
            lcd_puts_rpm(rpm, pin);
            interval_RA[pin][0] = 0;
            interval_RA[pin][1] = 0;
        }
        __delay_ms(500);
    }
    return;
}
