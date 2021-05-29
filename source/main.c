/* Author: Benjamin Denzler
 * Lab Section: 21
 * Assignment: Lab #11
 * Exercise Description: Pong
 *
 * I acknowledge all content contained herein, excluding template or example
 * code is my own original work.
 *
 *  Demo Link: <insert>
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "../header/timer.h"
#include "../header/scheduler.h"
#include "../header/keypad.h"

// Pattern on a row: 0 = on, 1 = off (invert)
// Which rows to display on: 1 = on, 0 = off
// 0x01 = row 1 (top), 0x80 = row 8 (bottom)

// Matrix info
const unsigned char numRows = 8;

// Paddle variables
const unsigned char paddleTopPos = 0xE0;
const unsigned char paddleBotPos = 0x07;
unsigned char leftPaddlePattern = 0x1C;
unsigned char rightPaddlePattern = 0x1C;

// DEBUGGING: Show output of any char on LEDs
void displayChar(unsigned char var) {
    switch (var) {
        case 0: PORTB = 0x1F; break;
        case 1: PORTB = 0x01; break;
        case 2: PORTB = 0x02; break;
        case 3: PORTB = 0x03; break;
        case 4: PORTB = 0x04; break;
        case 5: PORTB = 0x05; break;
        case 6: PORTB = 0x06; break;
        case 7: PORTB = 0x07; break;
        case 8: PORTB = 0x08; break;
        case 9: PORTB = 0x09; break;
        case 10: PORTB = 0x0A; break;
        case 11: PORTB = 0x0B; break;
        case 12: PORTB = 0x0C; break;
        case 13: PORTB = 0x0D; break;
        case 14: PORTB = 0x0E; break;
        case 15: PORTB = 0x00; break;
        case 16: PORTB = 0x0F; break;
        default: PORTB = 0x1B; break;  // Should never occur, middle LED off
    }
}

// Demo code for LED matrix
enum Demo_States { shift_demo };
int Demo_Tick(int state) {
    static unsigned char pattern = 0x80;  // Pattern on a row: 0 = on, 1 = off (invert)
    static unsigned char row = 0x01;      // Which rows to display on: 1 = on, 0 = off
                                          // 0x01 = row 1 (top), 0x80 = row 8 (bottom)

    switch (state) {    // Transitions
        case shift_demo: break;
        default: state = shift_demo; break;
    }
    switch (state) {    // Actions
        case shift_demo:
            if ((pattern == 0x01) && (row == 0x80)) {
                pattern = 0x80;
                row = 0x01;
            }
            else if (pattern == 0x01) {
                pattern = 0x80;
                row = row << 1;
            }
            else {
                pattern = pattern >> 1;
            }
        default: break;
    }
    PORTC = ~pattern;
    PORTD = row;
    return state;
}

enum DemoSprite_States { shift_sprite };
int DemoSprite_Tick(int state) {
    unsigned char rows[8] = { 0x00, 0x24, 0x24, 0x00, 0x42, 0x42, 0x3C, 0x00 };  // Pattern per row
    static unsigned char pattern = 0x00;    // Start with row[0]
    static unsigned char row = 0x01;        // Start at row 1
    static unsigned char i = 0;

    switch (state) {    // Transitions
        case shift_sprite: break;
        default: state = shift_sprite; break;
    }
    switch (state) {    // Actions
        case shift_sprite:
            if (i != numRows) {
                PORTC = ~pattern;
                PORTD = row;
                if (i == 7) { ++i; }
                else { pattern = rows[++i]; } 
                row <<= 1;
            }
            else {
                i = 0;
                pattern = 0x00;
                row = 0x01;
            }
            break;
        default: break;
    }
    return state;
}

enum RightPaddleInput_States { RPI_Wait, RPI_Up, RPI_Down };
int RightPaddleInput_Tick(int state) {
    unsigned char tmpPA0 = (~PINA) & 0x01;  // PA0 = right paddle up
    unsigned char tmpPA1 = (~PINA) & 0x02;  // PA1 = right paddle down

    switch (state) {    // Transitions
        case RPI_Wait:
            if ((rightPaddlePattern != paddleTopPos) && tmpPA0) {
                rightPaddlePattern <<= 1;
                state = RPI_Up;
            }
            else if ((rightPaddlePattern != paddleBotPos) && tmpPA1) {
                rightPaddlePattern >>= 1;
                state = RPI_Down;
            }
            break;

        case RPI_Up:
            if (tmpPA0) { state = RPI_Up; }
            else if (!tmpPA0) { state = RPI_Wait; }
            break;

        case RPI_Down:
            if (tmpPA1) { state = RPI_Down; }
            else if (!tmpPA1) { state = RPI_Wait; }
            break;

        default: state = RPI_Wait; break;
    }
    return state;
}

enum PaddlesOutput_States { PO_DisplayRight, PO_DisplayLeft };
int PaddlesOutput_Tick(int state) {
    switch (state) {    // Transitions
        case PO_DisplayRight: state = PO_DisplayLeft; break;
        case PO_DisplayLeft: state = PO_DisplayRight; break;
        default: state = PO_DisplayRight; break;
    }
    switch (state) {    // Actions
        case PO_DisplayRight:
            PORTC = ~rightPaddlePattern;
            PORTD = 0x01;
            break;
        case PO_DisplayLeft:
            PORTC = ~leftPaddlePattern;
            PORTD = 0x80;
            break;
        default: break;
    }
    return state;
}

int main(void) {
    DDRA = 0x00; PORTA = 0xFF;  // Pin A is input
    DDRC = 0xFF; PORTC = 0x00;  // Pin C is output (matrix cols, GROUND)
    DDRD = 0xFF; PORTD = 0x00;  // Pin D is output (matrix rows, OUTPUT)

    // Array of tasks
    static task task1, task3;
    task* tasks[] = { &task1, &task3 };
    const unsigned short numTasks = (sizeof(tasks) / sizeof(task*));

    const char start = -1;

    // Task 1 (Demo_Tick)
    task1.state = start;
    task1.period = 1;
    task1.elapsedTime = task1.period;
    task1.TickFct = &DemoSprite_Tick;

    // // Task 2 (RightPaddleInput_Tick)
    // task2.state = start;
    // task2.period = 10;
    // task2.elapsedTime = task2.period;
    // task2.TickFct = &RightPaddleInput_Tick;

    // Task 3 (PaddlesOutput_Tick)
    task3.state = start;
    task3.period = 1;
    task3.elapsedTime = task3.period;
    task3.TickFct = &PaddlesOutput_Tick;

    // Find GCD for period
    unsigned short i = 0;
    unsigned long GCD = tasks[0]->period;
    for (i = 1; i < numTasks; ++i) {
        GCD = findGCD(GCD, tasks[i]->period);
    }

    TimerSet(GCD);
    TimerOn();

    while (1) {
        for (i = 0; i < numTasks; ++i) {
            if (tasks[i]->elapsedTime >= tasks[i]->period) {
                tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
                tasks[i]->elapsedTime = 0;
            }
            tasks[i]->elapsedTime += GCD;
        }
        while (!TimerFlag);
        TimerFlag = 0;
    }
    return 0;
}

/*
WORKS CITED
-------------------------------------
Pinout for the 8x8 LED matrix being used:
https://www.zpag.net/Electroniques/Arduino/8x8_dot_matrix_1588bs.html

Got help from Ashley Pang (apang024) to create a method for time-multiplexed output
*/
