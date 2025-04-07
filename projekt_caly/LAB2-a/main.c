/************************************************************************/
/* INCLUDE                                                              */
/************************************************************************/
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "makra.h"
#include "uart/uart.h"
#include "nmea_parse/nmea_parser.h"

/************************************************************************/
/* DEFINES                                                              */
/************************************************************************/

#define NMEA_BUF_SIZE 82

/************************************************************************/
/* VARIABLES                                                            */
/************************************************************************/

FILE uart_str = FDEV_SETUP_STREAM(printCHAR, NULL, _FDEV_SETUP_RW);//soubor pro stdout
volatile char nmea_buffer[NMEA_BUF_SIZE];
volatile uint8_t nmea_index = 0;
volatile uint8_t nmea_mode = 0; // nový p?epína?

/************************************************************************/
/* PROTOTYPES                                                           */
/************************************************************************/

void board_init();

/************************************************************************/
/* FUNCTIONS                                                            */
/************************************************************************/
void board_init(){
	cli(); //disable interrupts - defaultne je sice vyple, ale pro jistotu
	
	UART_init(38400); //baudrate 38400b/s
	
	UCSR1B |= (1 << RXCIE1);// UART receive interrupt enable
	//sbi(UCSR1B,RXCIE1);
	stdout = &uart_str; //standartní výstup/std output
	
	//LED0 vystup/output
	sbi(DDRB,4);
	sbi(PORTB,4); //turn the LED0 down

	//LED1
	sbiX(DDRB,5);
	sbi(PORTB,5);
	
	//Tlacitka vstup/buttons as input
	cbi(DDRE,4);
	//Tlacitka pull-up
	sbi(PORTE,4);
	
	sbi(EIMSK,4); //povolit preruseni INT5 - tlacitko button0 /INT5 enable
	sbi(EICRB,ISC41);//nastupna hrana /rising edge
	sbi(EICRB,ISC40);//nastupna hrana
	
	sei(); // enable interrupts
}

void simulate_gps_input(const char *sentence)
{
    for (uint8_t i = 0; sentence[i] != '\0'; i++) {
        char received = sentence[i];

        UART_SendChar(received); // echo simulace

        if (received == '\n') {
            nmea_buffer[nmea_index] = '\0';

            NMEAData data;
            NMEA_ParseSentence(nmea_buffer, &data);

            if (data.time[0] != '\0') {
                UART_SendString("\r\nZpracovany cas: ");
                UART_SendString(data.time);
                UART_SendString("\r\n");
            }

            nmea_index = 0;
        } else {
            if (nmea_index < NMEA_BUF_SIZE - 1) {
                nmea_buffer[nmea_index++] = received;
            } else {
                nmea_index = 0; // p?ete?ení
            }
        }

        _delay_ms(10); // zpožd?ní mezi znaky jako na sériové lince
    }
}

/************************************************************************/
/* MAIN                                                                 */
/************************************************************************/
int main(void)
{
	board_init();
	stdout = fdevopen(printCHAR, NULL);

	UART_SendString("Start GPS simulace...\r\n");
	_delay_ms(3000);

	while (1) {
		UART_SendString("\r\n--- Simuluji novou NMEA vetu ---\r\n");


simulate_gps_input("$GPGGA,101403.308,4927.930,N,01722.884,E,1,12,1.0,0.0,M,0.0,M,,*62\n");
_delay_ms(1000);
simulate_gps_input("$GPGGA,111703.966,4927.237,N,01723.409,E,1,12,1.0,0.0,M,0.0,M,,*66\n");

		_delay_ms(9000); // simulace 10 sekundové periody
	}
}
/************************************************************************/
/* INTERRUPTS                                                           */
/************************************************************************/


ISR(USART1_RX_vect)
{
	char received = UDR1;
	UART_SendChar(received); // echo

	if (received == '\n') {
		nmea_buffer[nmea_index] = '\0'; // ukon?ení ?et?zce

		NMEAData data;
		NMEA_ParseSentence((char*)nmea_buffer, &data);

		if (data.time[0] != '\0') {
			UART_SendString("\r\nZpracovany cas: ");
			UART_SendString(data.time);
			UART_SendString("\r\n");
		}

		nmea_index = 0;
	} else {
		if (nmea_index < NMEA_BUF_SIZE - 1) {
			nmea_buffer[nmea_index++] = received;
		} else {
			nmea_index = 0; // p?ete?ení – reset
		}
	}
}


