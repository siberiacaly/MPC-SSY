#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "nmea_parser.h"

#define BAUD 38400
#define MYUBRR (F_CPU/16/BAUD-1)

void UART_init(uint16_t Baudrate) {
	UBRR1H = (uint8_t)(Baudrate >> 8);
	UBRR1L = (uint8_t)Baudrate;
	UCSR1B |= (1 << RXEN1) | (1 << TXEN1) | (1 << RXCIE1);
	UCSR1C |= (1 << UCSZ11) | (1 << UCSZ10);
	sei(); 
}

void UART_SendChar(uint8_t data) {
	while (!(UCSR1A & (1 << UDRE1)));
	UDR1 = data;
}

void UART_SendString(char *text) {
	while (*text) {
		if (*text == '\n') {
			UART_SendChar('\r');
		}
		UART_SendChar(*text);
		text++;
	}
}


int uart_putchar(char c, FILE *stream) {
	if (c == '\n') {
		uart_putchar('\r', stream);
	}
	UART_SendChar(c);
	return 0;
}
FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
void init_uart_stdio(void) {
	stdout = &uart_output;
}

int main(void) {
	UART_init(MYUBRR);
	init_uart_stdio();
	
	_delay_ms(100);
	UART_SendString("NMEA Parser Test Program\r\n");
	char test_sentence1[] = "$GPGGA,111912.636,4928.006,N,01722.742,E,1,12,1.0,0.0,M,0.0,M,,*60";
	char test_sentence2[] = "$GPRMC,111912.636,A,4928.006,N,01722.742,E,,,310325,000.0,W*72";
	
	NMEAData data;
	
	// Zpracování první v?ty GPGGA
	NMEA_ParseSentence(test_sentence1, &data);
	printf("Parsed GPGGA Sentence:\r\n");
	printf("Sentence Type: %s\r\n", data.sentence_type);
	printf("Time: %s\r\n\r\n ", data.time);
	
	// Zpracování druhé v?ty GPRMC
	NMEA_ParseSentence(test_sentence2, &data);
	printf("Parsed GPRMC Sentence:\r\n");
	printf("Sentence Type: %s\r\n", data.sentence_type);
	printf("Time: %s\r\n", data.time);
	
	// Nekone?ná smy?ka, aby program neskon?il
	while (1) {
		// M?žete sem p?ípadn? vložit další zpracování nebo pauzu
	}
	
	return 0;
}
