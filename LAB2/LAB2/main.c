#include <avr/io.h>
#include <avr/interrupt.h>
#include "makra.h"
#include <util/delay.h>


#define BAUD 38400
#define MYUBRR (F_CPU/16/BAUD-1)
#define BUTTON1_PIN PINE,5  
#define BUTTON1_PRESSED !(PINE & (1 << 5))  

volatile uint8_t receivedChar;
volatile uint8_t buttonPressed = 0;
void UART_init(uint16_t Baudrate) {
    UBRR1H = (uint8_t)(Baudrate >> 8);
    UBRR1L = (uint8_t)Baudrate;
    sbi(UCSR1B, RXEN1);
    sbi(UCSR1B, TXEN1);
    sbi(UCSR1B, RXCIE1); 
    sbi(UCSR1C, UCSZ11);
    sbi(UCSR1C, UCSZ10);
    sei(); 
}

void UART_SendChar(uint8_t data) {
    while (!(UCSR1A & (1 << UDRE1)));
    UDR1 = data;
}

void UART_SendString(char *text) {
    while (*text) {
        UART_SendChar(*text);
        text++;
    }
}

uint8_t UART_GetChar(void) {
    while (!(UCSR1A & (1 << RXC1)));
    return UDR1;
}

ISR(USART1_RX_vect) {
    receivedChar = UDR1;
}

void blinkLED2(void) {
    for (int i = 0; i < 3; i++) {
        LED2ON;
        _delay_ms(500);
        LED2OFF;
        _delay_ms(500);
    }
}
void clearScreen() {
	UART_SendString("\x1b[2J");  
	UART_SendString("\x1b[H");  
}
void showMenu(void) {
	clearScreen(); 
	UART_SendString("\x1b[32m");  
	UART_SendString("\nMENU:\n");
	UART_SendString("0 - Exit\n");
	UART_SendString("1 - Small letters\n");
	UART_SendString("2 - Capital letters\n");
	UART_SendString("3 - Blink LED2\n");
	UART_SendString("\x1b[0m");  
}

void checkButtonPolling(void) {
	if (BUTTON1_PRESSED) {
		UART_SendString("Button pressed\n"); 
		_delay_ms(50);  
		if (BUTTON1_PRESSED) {
			UART_SendChar('1');  
			while (BUTTON1_PRESSED); 
	}
}


ISR(INT5_vect) {
	if (BUTTON1_PRESSED && !buttonPressed) {
		UART_SendChar('1');
		buttonPressed = 1;
	}
	else if (!BUTTON1_PRESSED && buttonPressed) {
		buttonPressed = 0;
	}
	EIFR |= (1 << INTF5);
}



void button_interrupt_init(void) {
	sbi(PORTE, 5);
	cbi(DDRE, 5);
	EICRB |= (1 << ISC50);
	EIMSK |= (1 << INT5);
}

int main(void) {
    sbi(PORTE, 5);  
    cbi(DDRE, 5);   

    UART_init(MYUBRR);
	button_interrupt_init();
    UART_SendString("UART initialized. Starting program...\n");
    showMenu();
    
	sei();  
	
    while (1) {
		        uint8_t choice = UART_GetChar();
        if (choice) {
            UART_SendChar(choice);
            UART_SendString("\n");

            switch (choice) {
                case '0':
                    UART_SendString("Exiting...\n");
                    return 0;
                case '1':
                    for (char c = 'a'; c <= 'z'; c++) UART_SendChar(c);
                    UART_SendString("\n");
                    break;
                case '2':
                    for (char c = 'A'; c <= 'Z'; c++) UART_SendChar(c);
                    UART_SendString("\n");
                    break;
                case '3':
                    sbi(DDRB, 6);
                    blinkLED2();
                    break;
                default:
                    UART_SendString("Invalid choice!\n");
            }
            showMenu();
        }
    }
}
