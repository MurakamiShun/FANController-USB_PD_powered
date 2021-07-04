/*
 * File:   main.c
 * Author: MurakamiShun
 *
 * Created on 2021/07/03, 16:39
 */
#pragma config CLKOUTEN = OFF, PLLEN = ON, FOSC = INTOSC
#pragma config LVP = OFF, MCLRE = OFF
#pragma config CP = ON, CPD = ON
#pragma config BOREN = ON, STVREN = OFF, IESO = OFF, FCMEN = OFF, BORV = LO
#pragma config WDTE = OFF, PWRTE = OFF,

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#define _XTAL_FREQ 32000000L

uint8_t ADC_RA1 = 0;
uint8_t ADC_RA2 = 0;

void init_tmr1(void){
    GIE = 1; // enable interrupt
    PEIE = 1;
    T1CON = 0x00;  // focs
    T1CONbits.T1CKPS = 0b00; // timer1 8MHz/2
    T1CONbits.TMR1ON = 1; // enable timer1
    // count up to 20;
    CCPR1H = 0x00;
    CCPR1L = 20;
    CCP1CONbits.CCP1M = 0b1011; // compare and reset TMR1
    CCP1IE = 1; // enable timer1 interrupt
    CCP1IF = 0;
}

uint8_t reset_count = 0;

void interrupt inter_tmr(void){
    // 25kHz
    if(CCP1IF){
        reset_count += 0b11100;
        //reset_count += 16;
        RA5 = (reset_count < ADC_RA1);
        RA4 = (reset_count < ADC_RA2);
        CCP1IF = 0;
    }
}

void init(void){
    OSCCONbits.IRCF = 0b1110; // 8MHz
    OSCCONbits.SPLLEN = 1; // 4xPLL
    OSCCONbits.SCS = 0b00;
    ANSELA = 0b00000110; // RA1 RA2 are analog
    TRISA = 0b00000110;  // RA1 RA2 are input
    WPUA = 0b00000000;  // pull up
    nWPUEN = 1; // disable pull up
    PORTA = 0x00;
    
    // ADC
    ANSA1 = 1;
    ANSA2 = 1;
    ADIE = 0; // disable ADC interrupt
    ADCON1bits.ADFM = 0; // left justify
    ADCON1bits.ADCS = 0b110; // Fosc/64
    ADCON1bits.ADPREF = 0b00; // reference is Vdd
    ADCON0bits.ADON = 1; // ADC ON
    init_tmr1();
}

void main(void) {
    init();
    
    while(1){
        // RA1
        ADCON0bits.CHS = 0b00001; // RA1
        __delay_us(2);
        ADCON0bits.GO = 1;
        while(ADCON0bits.GO);
        ADC_RA1 = ADRESH;
        __delay_us(2);
        // RA2
        ADCON0bits.CHS = 0b00010; // RA2
        __delay_us(2);
        ADCON0bits.GO = 1;
        while(ADCON0bits.GO);
        ADC_RA2 = ADRESH;
        __delay_us(2);
    }
    return;
}
