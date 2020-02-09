/* 
 * File: main.c
 * Description: Memory Hack for Edifier Sound System
 */

#define COMPA_VECT  TIMER1_COMPA_vect  
#define AVR_ATmega8

/* Define UART buad rate here */
#define UART_BAUD_RATE      9600   
#define TWI_SLAVE_ADDRESS   0x88

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdlib.h>         //nötig für Zahlumwandlung mit itoa
#include "uart/uart.h"
#include "irsnd/irsnd.h" 
#include "twi/twislave.h" 
#include "edifier.h"

#define NEC_MUTE        0x41BE
#define NEC_VOL_UP      0x06F9
#define NEC_VOL_DOWN    0x49B6
#define NEC_IN_PC       0x03FC
#define NEC_IN_AUX      0x02FD
#define NEC_BASS_UP     0x07F8
#define NEC_BASS_DOWN   0x5CA3
#define NEC_TREB_UP     0x09F6
#define NEC_TREB_DOWN   0x1EE1

#define EEPROM_VOLUME_ADDR  (uint8_t*) 0
#define EEPROM_INPUT_ADDR   (uint8_t*) 1
#define EEPROM_TREB_ADDR    (uint8_t*) 2
#define EEPROM_BASS_ADDR    (uint8_t*) 3

volatile uint8_t newPackage = 0;

const Edifier Edifier_default = {30, INPUT_PC, 0, 0, SYSTEM_ON} ;

Edifier currentState = Edifier_default;
Edifier desiredState = Edifier_default;

void timer1_init (void)
{                                                                       // ATmegaXX:
    OCR1A   =  (F_CPU / F_INTERRUPTS) - 1;                                  // compare value: 1/15000 of CPU frequency
    TCCR1B  = (1 << WGM12) | (1 << CS10);                                   // switch CTC Mode on, set prescaler to 1
    TIMSK   = 1 << OCIE1A;                                                  // OCIE1A: Interrupt by timer compare
}
void timer1_deinit (void) {
    TCCR1B  &= ~(1 << WGM12);                                               // switch CTC Mode on, set prescaler to 1
    TIMSK  &= ~(1 << OCIE1A);                                               // OCIE1A: Interrupt by timer compare    
}

void adc_init(void) {    
    ADMUX = (1<<REFS0);               // Set Reference to AVCC and input to ADC0
    ADCSRA = (1<<ADFR)|(1<<ADEN)      // Enable ADC, set prescaler to 128
            |(1<<ADPS2)|(1<<ADPS1)
            |(1<<ADPS0);              // Fadc=Fcpu/prescaler=8000000/128=62.5kHz
                                      // Fadc should be between 50kHz and 200kHz
    ADCSRA |= (1<<ADSC);              // Start the first conversion
}
void adc_deinit(void) {
    ADCSRA = 0x00;
}

//Hilfsfunktion, um Zahlen über das UART auszugeben
void uart_puti( const int val )
{
    char buffer[8];
    uart_puts( itoa(val, buffer, 10) );
}

void printEdifier(Edifier* p, const char* prefix) {
    uart_puts(prefix);
    uart_puts(" - Vol:");
    uart_puti(p->volume);
    uart_puts(" In:");
    uart_puti(p->input);
    uart_puts(" Treb:");
    uart_puti(p->treb);
    uart_puts(" Bass:");
    uart_puti(p->bass);
    uart_puts("\r\n");
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * timer 1 compare handler, called every 1/10000 sec
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
ISR(COMPA_VECT)                                                             // Timer1 output compare A interrupt service routine, called every 1/15000 sec
{
    (void) irsnd_ISR();                                                     // call irsnd ISR
} 

ISR (TWI_vect) 
{    
    // recv finished for one package
    if (twislave_ISR() == TW_SR_STOP) {
        edi_decode_package((uint8_t*)i2cpackage, &currentState);
        newPackage = 1;
   } 
}

void read_eeprom() {
    cli();
    desiredState.volume = eeprom_read_byte(EEPROM_VOLUME_ADDR);
    desiredState.input = eeprom_read_byte(EEPROM_INPUT_ADDR);
    desiredState.treb = (int8_t)eeprom_read_byte(EEPROM_TREB_ADDR);
    desiredState.bass = (int8_t)eeprom_read_byte(EEPROM_BASS_ADDR);
    sei();
}

void write_eeprom() {
    cli();
    eeprom_write_byte(EEPROM_VOLUME_ADDR, currentState.volume);
    eeprom_write_byte(EEPROM_INPUT_ADDR, currentState.input);
    eeprom_write_byte(EEPROM_TREB_ADDR, (uint8_t)currentState.treb);
    eeprom_write_byte(EEPROM_BASS_ADDR, (uint8_t)currentState.bass);
    sei();
}

void sendCommand(uint16_t command)
{
    static IRMP_DATA irmp_data = {IRMP_NEC_PROTOCOL, ~0x00FF, 0, 0};

    irmp_data.command = ~command;
    irsnd_send_data (&irmp_data, TRUE);
}

void init_eeprom() {
    //init eeprom and do sanity check
    read_eeprom();
    printEdifier(&desiredState, "Init Desired");
    if (desiredState.volume > 60 | desiredState.input > 1 | desiredState.treb > 7
        | desiredState.treb < -7 | desiredState.bass > 10 | desiredState.bass < -10) {
        printEdifier(&currentState, "Error");
        write_eeprom();
        read_eeprom();
        printEdifier(&desiredState, "Reset");
    }
}

int main (void) 
{    
	IRMP_DATA irmp_data;
    uint8_t load = 1;
    uint8_t save = 1;
	uart_init((UART_BAUD_SELECT((UART_BAUD_RATE),F_CPU)));
    uart_puts("Startup!\r\n");

    sei();
    init_eeprom();

    _delay_ms(100);
    cli();
    init_twi_slave(TWI_SLAVE_ADDRESS);
    irsnd_init();                                                           // initialize irsnd
    timer1_init();                                                          // initialize timer
    sei();    

    _delay_ms(1000);

    while (load) {
        if (currentState.input != desiredState.input) {
            if (desiredState.input == INPUT_PC) {
                sendCommand(NEC_IN_PC);
            }
            else {
                sendCommand(NEC_IN_AUX);                
            }
        }
        else if (currentState.treb != desiredState.treb) {
            if (currentState.treb > desiredState.treb) {
                sendCommand(NEC_TREB_DOWN);
            }
            else {
                sendCommand(NEC_TREB_UP);
            }
        }
        else if (currentState.bass != desiredState.bass) {
            if (currentState.bass > desiredState.bass) {
                sendCommand(NEC_BASS_DOWN);
            }
            else {
                sendCommand(NEC_BASS_UP);
            }
        }
        else if (currentState.volume != desiredState.volume && desiredState.volume != 0) {
            if (currentState.volume > desiredState.volume) {
                sendCommand(NEC_VOL_DOWN);
            }
            else {
                sendCommand(NEC_VOL_UP);
            }
        }
        else {
            load = 0;
        }
        _delay_ms(100);
    }

    uart_puts("Finished!\r\n");
    timer1_deinit();
    irsnd_deinit();

    adc_init();
    _delay_ms(100);

    while(save)                          // Endless loop
    {                                 // main() will never be left
        if(ADC == 1023){
            adc_deinit();
            twi_slave_deinit();

            write_eeprom();
            uart_puts("PowerLos!\r\n");
            printEdifier(&currentState, "Saved");
            save = 0;
            // power loss detected. store settings now
        }
    }

    for(;;);
}
