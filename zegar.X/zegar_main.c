#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


int tryb = 0;
int chosen_digit = 5;
int off = 7;
// Definicje segmentów (zale?nie od po??czenia hardware)
const uint8_t SEGMENT_MAP[11] = {
    0b11000000, // 0
    0b11111001, // 1
    0b10100100, // 2
    0b10110000, // 3
    0b10011001, // 4
    0b10010010, // 5
    0b10000010, // 6
    0b11111000, // 7
    0b10000000, // 8
    0b10010000, // 9
    0b11111111  // brak
};
const uint8_t wybory[6] = {
    0b00100000, // sekunda
    0b00010000, // sekundy
    0b00001000, // minuta
    0b00000100, // minuty
    0b00000010, // godzina
    0b00000001  // godziny
};

int teraz = 0;

// Zmienne globalne do przechowywania czasu
volatile uint8_t seconds = 0;
volatile uint8_t minutes = 0;
volatile uint8_t hours = 0;

// Zmienne do multipleksowania wy?wietlaczy
volatile uint8_t current_digit = 0;
uint8_t digits[6] = {0, 0, 0, 0, 0, 0};

// Funkcja aktualizuj?ca cyfry do wy?wietlenia
void update_digits() {
    digits[5] = hours / 10;
    digits[4] = hours % 10;
    digits[3] = minutes / 10;
    digits[2] = minutes % 10;
    digits[1] = seconds / 10;
    digits[0] = seconds % 10;
}
ISR(TIMER0_OVF_vect) {
    // Kod obs?uguj?cy przerwanie Timer0 Overflow
    display_update();
}
// Przerwanie Timera1 do odmierzania 1 sekundy
ISR(TIMER1_COMPA_vect) {
    if (tryb == 0){
        seconds++;
        if (seconds >= 60) {
            seconds = 0;
            minutes++;
            if (minutes >= 60) {
                minutes = 0;
                hours++;
                if (hours >= 24) {
                    hours = 0;
                }
            }
        }
        update_digits();
    }
    else {
        if (off>6) {
            off = chosen_digit;
        } else {
            off = 7;
        }
    }
    
    
}

void timer0_init() {
    TCCR0 |= (1 << CS01) | (1 << CS00);  // Prescaler 1024
    TIMSK |= (1 << TOIE0);  // W??czenie przerwania od Compare Match
}
// Funkcja inicjalizuj?ca Timer1
void timer1_init() {
    TCCR1B |= (1 << WGM12); // Tryb CTC
    OCR1A = 31675;          // Ustawienie cz?stotliwo?ci (16MHz / 1024 preskaler / 1 Hz)
    TCCR1B |= (1 << CS12); //| (1 << CS10); // Preskaler 1024
    TIMSK |= (1 << OCIE1A); // W??cz przerwanie CTC
}

// Funkcja inicjalizuj?ca porty
void ports_init() {
    DDRD = 0xFF; // Port B jako wyj?cie (segmenty)
    DDRC = 0x3F; // Port D (PD0-PD5) jako wyj?cie (cyfry)
    PORTD = 0x00;
    PORTC = 0x00;
    DDRB &= ~((1 << PB0) | (1 << PB1));  // PB0 i PB1 jako wej?cia
    PORTB |= (1 << PB0) | (1 << PB1);    // W??czenie pull-up dla PB0 i PB1
}

// Funkcja multipleksuj?ca wy?wietlacze
void display_update() {
    PORTC = 0x00; // Dezaktywacja wy?wietlacza
    PORTD = 0xFF;
    PORTC = wybory[(current_digit)]; // Aktywacja wybranego wy?wietlacza
    if (off > 6) {
    PORTD = SEGMENT_MAP[digits[(current_digit + 3) % 6]]; // Wy?wietlenie cyfry
    } else if (off != (current_digit+3) % 6) {
         PORTD = SEGMENT_MAP[digits[(current_digit + 3) % 6]]; // Wy?wietlenie cyfry
    } else{
        PORTD = 0b11111111;
    }
   // _delay_ms(1); // Krótki czas na wy?wietlenie
   
    current_digit++;
    if (current_digit >= 6) {
        current_digit = 0;
    }
    if (current_digit == 2 | current_digit == 0 ) {
        PORTD = PORTD & 0b01111111;
    }
}

void change_digit() {
    if (tryb == 0){
        tryb = 1;
        chosen_digit = 5;
    }else {
        if(chosen_digit == 0) {
            tryb = 0;
        }else{
        chosen_digit = chosen_digit - 1;
        }
    }
    _delay_ms(2000);// /8
}

void change_time(){
    if (tryb == 1) {
        switch (chosen_digit) {
                case 5:
                    hours = hours + 10;
                    if(hours > 24) {
                        hours = hours - 30;
                    }
                    break;
                case 4:
                    if ((hours % 10) == 9) {
                        hours = hours - 9;
                    } else if (hours == 24) {
                        hours = 20;
                    } else {
                        hours++;
                    }
                    break;
                case 3:
                    if (minutes >49) {
                        minutes = minutes % 10;
                    } else {
                        minutes = minutes + 10;
                    }
                    break;
                case 2:
                    if ((minutes % 10) == 9) {
                        minutes = minutes - 9;
                    } else if (minutes == 59) {
                        minutes = 50;
                    } else {
                        minutes++;
                    }
                    break;
                case 1:
                    if (seconds > 49) {
                        seconds = seconds % 10;
                    } else {
                        seconds = seconds + 10;
                    }
                    break;
                case 0:
                    if ((seconds % 10) == 9) {
                        seconds = seconds - 9;
                    } else if (seconds == 59) {
                        seconds == 50;
                    } else {
                        seconds++;
                    }
                    break;
        }
        
    }
    update_digits();
    _delay_ms(2000);// /8
}

int main(void) {
    ports_init();
    timer0_init();
    timer1_init();
    sei(); // W??czenie globalnych przerwa?
    
    

    while (1) {
         // Aktualizacja wy?wietlacza
        if (!(PINB & (1 << PB0))) {  // Sprawdzenie, czy przycisk na PB0 jest wci?ni?ty (logiczne 0)
            change_digit();
        }
        if (!(PINB & (1 << PB1))) {  // Sprawdzenie, czy przycisk na PB1 jest wci?ni?ty (logiczne 0)
            change_time();
        }
    }
    return 0;
}
