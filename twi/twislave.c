#include <util/twi.h> 	    //enthält z.B. die Bezeichnungen für die Statuscodes in TWSR
#include <avr/interrupt.h>  //dient zur Behandlung der Interrupts
#include <stdint.h>         //definiert den Datentyp uint8_t 
#include <string.h>
#include "twislave.h"

//%%%%%%%% Globale Variablen, die vom Hauptprogramm genutzt werden %%%%%%%%
/*Der Buffer, in dem die Daten gespeichert werden. 
Aus Sicht des Masters läuft der Zugrif auf den Buffer genau wie bei einem I2C-EEPROm ab.
Für den Slave ist es eine globale Variable
*/
volatile uint8_t i2cdata[i2c_buffer_size];

/*Initaliserung des TWI-Inteface. Muss zu Beginn aufgerufen werden, sowie bei einem Wechsel der Slave Adresse
Parameter adr: gewünschte Slave-Adresse
*/
void init_twi_slave(uint8_t adr)
{
	TWAR= adr; //Adresse setzen
	TWCR &= ~(1<<TWSTA)|(1<<TWSTO);
	TWCR|= (1<<TWEA) | (1<<TWEN)|(1<<TWIE); 
}

void twi_slave_deinit() {
    TWCR &= (1<<TWEN);
}


//Je nach Statuscode in TWSR müssen verschiedene Bitmuster in TWCR geschreiben werden(siehe Tabellen im Datenblatt!). 
//Makros für die verwendeten Bitmuster:

//ACK nach empfangenen Daten senden/ ACK nach gesendeten Daten erwarten
#define TWCR_ACK TWCR = (1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC);  

//NACK nach empfangenen Daten senden/ NACK nach gesendeten Daten erwarten     
#define TWCR_NACK TWCR = (1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC);

//switch to the non adressed slave mode...
#define TWCR_RESET TWCR = (1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(1<<TWEA)|(0<<TWSTA)|(1<<TWSTO)|(0<<TWWC);  


/*ISR, die bei einem Ereignis auf dem Bus ausgelöst wird. Im Register TWSR befindet sich dann 
ein Statuscode, anhand dessen die Situation festgestellt werden kann.
*/
uint8_t twislave_ISR()
{
    static uint8_t rcv_count = 0;
    uint8_t statuscode = TW_STATUS;

    switch (TW_STATUS) //TWI-Statusregister prüfen und nötige Aktion bestimmen 
    {
        case TW_SR_SLA_ACK:   // 0x60 Slave Receiver, Slave wurde adressiert 
            rcv_count = 0;    
            TWCR_ACK;
            break;        
        case TW_SR_DATA_ACK: // 0x80 Slave Receiver, ein Datenbyte wurde empfangen 
        	if (rcv_count < i2c_buffer_size)
        	{
	            i2cdata[rcv_count] = TWDR; //Empfangene Daten auslesen
	            rcv_count++;        		
        	}
            TWCR_ACK;
            break;
        case TW_SR_STOP:
            memcpy((uint8_t *)i2cpackage, (uint8_t *)i2cdata, 8);
            TWCR_ACK;
            break;  
        default:    
            TWCR_RESET;
    }

    return statuscode;
} //end.ISR(TWI_vect)

////Ende von twislave.c////