#include <avr/io.h>
#include <avr/interrupt.h>
#include "makra.h"
#include <util/delay.h>
#include "timer.h"


#define BAUD 38400
#define MYUBRR (F_CPU/16/BAUD-1)
#define BUTTON1_PIN PINE,5  
#define BUTTON1_PRESSED !(PINE & (1 << 5))  

volatile uint8_t led1Blinking = 0;
volatile uint8_t pwm_strida = 50;
volatile uint8_t counter = 0;
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
		if (*text == '\n') {
			UART_SendChar('\r');
		}
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
	UART_SendString("4 - LED1 Control\n");
}

void showLED1Menu(void) {
	clearScreen();
	UART_SendString("\x1b[32m");  
	UART_SendString("\nLED1 Control Menu:\n");
	UART_SendString("1 - Start blinking\n");
	UART_SendString("2 - Stop blinking\n");
	UART_SendString("+ - Increase brightness\n");
	UART_SendString("- - Decrease brightness\n");
	UART_SendString("0 - Back to main menu\n");
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

void Timer1_init(void) {
	cli(); 

	TCCR1A = 0;  
	TCCR1B = 0;
	TIMSK1 = 0;

	OCR1A = OCR1A_VALUE;  

	TCCR1B |= (1 << WGM12); 
	sei();  
}

void Timer1_start(void) {
	TCCR1B |= PRESCALE;  
	TIMSK1 |= (1 << OCIE1A);  
	TCCR1A |= (1 << COM1A0);  
}

void Timer1_stop(void) {
	TCCR1B = 0;  
	TIMSK1 &= ~(1 << OCIE1A);  
}



void Timer2_fastpwm_start(uint8_t strida) {
	cli();  
	TCCR2A = 0; 
	TCCR2B = 0;
	TIMSK2 = 0; 

	// nastavit hodnotu pro PWM (OCR2A: st?ída 0-255)
	OCR2A = (255 * strida) / 100;

	// fast PWM mód:
	TCCR2A |= (1 << WGM21);  // WGM21 a WGM20 pro Fast PWM
	TCCR2A |= (1 << WGM20);
	
	// 1024 prescaler:
	TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);

	// povolit p?erušení, pokud bude pot?eba
	TIMSK2 |= (1 << TOIE2);
	
	// výstup na pin OC2A (PB4):
	TCCR2A |= (1 << COM2A1);

	sei();  // povolit globální p?erušení
}

void handleLED1MenuInput(uint8_t choice) {
    switch (choice) {
        case '1':  
            led1Blinking = 1;
            Timer1_start(); 
            UART_SendString("LED1 blinking started\n");
            break;
        case '2': 
            led1Blinking = 0;
            Timer1_stop(); 
            UART_SendString("LED1 blinking stopped\n");
            break;
        case '+':  
            if (pwm_strida < 100) {
                pwm_strida += 5;  
            }
            Timer2_fastpwm_start(pwm_strida); 
            UART_SendString("Increased LED1 brightness\n");
            break;
        case '-':  
            if (pwm_strida > 0) {
                pwm_strida -= 5;  
            }
            Timer2_fastpwm_start(pwm_strida);  
            UART_SendString("Decreased LED1 brightness\n");
            break;
        case '0':  
/*            showMenu();  // Only go back to the main menu when '0' is pressed*/
            break;
        default:
            UART_SendString("Invalid choice!\n");
    }
}


ISR(TIMER1_COMPA_vect) {
	if (led1Blinking) {
		PORTB ^= (1 << PORTB5);  
		} else {
		PORTB |= (1 << PORTB5);  
	}
}




int main(void) {
	DDRB |= (1 << DDB5); 
	  Timer1_init();
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
// 				case '1':
// 				for (char c = 'a'; c <= 'z'; c++) UART_SendChar(c);
// 				UART_SendString("\n");
// 				break;
// 				case '2':
// 				for (char c = 'A'; c <= 'Z'; c++) UART_SendChar(c);
// 				UART_SendString("\n");
// 				break;
				case '3':
				sbi(DDRB, 6);
				blinkLED2();
				break;
				case '4':
				showLED1Menu();
				uint8_t led1Choice = UART_GetChar();
				handleLED1MenuInput(led1Choice);
				break;
				default:
				UART_SendString("Invalid choice!\n");
			}
			showMenu();
		}
	}
}

