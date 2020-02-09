/*
Testprogramm für den Slave 
Der Buffer wird mit Werten gefüllt. Dann wird er fortlaufend über die serielle Schnittstelle ausgegeben.
Nun kann man dort sehen, wenn der Master einen Wert ändert
*/
#include <util/twi.h> 	    //enthält z.B. die Bezeichnungen für die Statuscodes in TWSR
#include <avr/interrupt.h>  //dient zur Behandlung der Interrupts
#include <stdint.h> 	    //definiert den Datentyp uint8_t
#include "twislave.h"
#include "uart.h"           //UART-Lib von P- Fleury
#include <stdlib.h>         //nötig für Zahlumwandlung mit itoa
#include <util/delay.h>


#define BAUD 9600 //Baudrate
#define SLAVE_ADRESSE 0x50 //Die Slave-Adresse

//Hilfsfunktion, um Zahlen über das UART auszugeben
void uart_puti( const int val )
{
    char buffer[8];
    uart_puts( itoa(val, buffer, 10) );

}

int main (void)
{
//TWI als Slave mit Adresse slaveadr starten
init_twi_slave(SLAVE_ADRESSE);

//i2cdatamit Werten füllen, die der Master auslesen und ändern kann
for(uint8_t i=0;i<i2c_buffer_size;i++)
	{
		i2cdata[i]=10+i;
	}

//Serielle Schnittstelle aktivieren
uart_init((UART_BAUD_SELECT((BAUD),F_CPU)));
uart_puts("I2C-Test\r\n");
uart_puts("Teste I2C-Slave mit Adresse "); uart_puti(SLAVE_ADRESSE);
uart_puts("\r\n");
uart_puts("\r\n"); //Leerzeile

//in einer Endlosschleife den Inhalt der Buffer ausgeben
while(1) 
{
	uart_puts("i2cdata:\r\n");
	for(uint8_t i=0;i<i2c_buffer_size;i++)
		{
			uart_puti(i2cdata[i]);
			uart_puts("\r\n");
		}
	uart_puts("\r\n");//leerzeile
_delay_ms(500);
} //end.while
} //end.main