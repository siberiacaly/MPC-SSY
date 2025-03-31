#ifndef TIMER1_H
#define TIMER1_H

#include <avr/io.h>
#include <avr/interrupt.h>

#define PRESCALE ((1 << CS10) | (1 << CS12))
#define OCR1A_VALUE 3905  

void Timer1_init(void);
void Timer1_start(void);
void Timer1_stop(void);

#endif
