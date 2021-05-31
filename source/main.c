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
#include <stdio.h>
#include <stdlib.h>
#include "../header/timer.h"
#include "../header/scheduler.h"
#include "../header/keypad.h"
#include "../header/pwm.h"

// Pattern on a row: 0 = on, 1 = off (invert)
// Which rows to display on: 1 = on, 0 = off
// 0x01 = row 1 (top), 0x80 = row 8 (bottom)

// PA0 = Right paddle up
// PA1 = Right paddle down
// PA2 = Start/reset

// Matrix info
const unsigned char numRows = 8;
unsigned char gameStart = 0;  // 0 = game reset, 1 = game start
unsigned char rows[8] = { 0x1C, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x1C };  // First, last are paddles

// Paddle variables
const unsigned char paddleTopPos = 0xE0;
const unsigned char paddleBotPos = 0x07;
#define rightPaddlePattern rows[0]
#define leftPaddlePattern rows[7]

// Resets row patters to default
void resetRows() {
    rows[0] = 0x1C; rows[1] = 0x00; rows[2] = 0x00;
    rows[3] = 0x08; rows[4] = 0x00; rows[5] = 0x00;
    rows[6] = 0x00; rows[7] = 0x1C;
}

enum StartReset_States { SR_Wait, SR_Update };
int StartReset_Tick(int state) {
    unsigned char tmpPA2 = (~PINA) & 0x04;  // PA2 = Start/reset

    switch (state) {    // Transitions
        case SR_Wait:
            if (tmpPA2) { 
                state = SR_Update; 
                gameStart = ~gameStart;
                if (!gameStart) { resetRows(); }
            }
            else { state = SR_Wait; }
            break;

        case SR_Update: 
            if (tmpPA2) { state = SR_Update; }
            else { state = SR_Wait; }
            break;

        default: state = SR_Wait; break;
    }
    return state;
}

// Get input, update right paddle position
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

// Outputs to each row, does not modify the patterns
enum Output_States { Display };
int Output_Tick(int state) {
    static unsigned char pattern = 0x00;    // Start with rows[0]
    static unsigned char row = 0x01;        // Start at row 1
    static unsigned char i = 0;

    switch (state) {    // Transitions
        case Display: break;
        default: state = Display; break;
    }
    switch (state) {    // Actions
        case Display:
            if (i != numRows) {
                PORTC = ~pattern;
                PORTD = row;
                if (i == 7) { ++i; }
                else { pattern = rows[++i]; } 
                row <<= 1;
            }
            else {
                i = 0;
                pattern = rows[0];
                row = 0x01;
            }
            break;
        default: break;
    }
    return state;
}

int main(void) {
    DDRA = 0x00; PORTA = 0xFF;  // Pin A is input (buttons)
    DDRB = 0x40; PORTB = 0xBF;  // PB6 is output (PWM), rest is input
    DDRC = 0xFF; PORTC = 0x00;  // Pin C is output (matrix cols, GROUND)
    DDRD = 0xFF; PORTD = 0x00;  // Pin D is output (matrix rows, OUTPUT)

    // Array of tasks
    static task task1, task2, task3;
    task* tasks[] = { &task1, &task2, &task3 };
    const unsigned short numTasks = (sizeof(tasks) / sizeof(task*));

    const char start = -1;

    // Task 1 (RightPaddleInput_Tick)
    task1.state = start;
    task1.period = 10;
    task1.elapsedTime = task1.period;
    task1.TickFct = &RightPaddleInput_Tick;

    // Task 2 (Output_Tick)
    task2.state = start;
    task2.period = 1;
    task2.elapsedTime = task2.period;
    task2.TickFct = &Output_Tick;

    // Task 3 (StartReset_Tick)
    task3.state = start;
    task3.period = 20;
    task3.elapsedTime = task3.period;
    task3.TickFct = &StartReset_Tick;

    // Find GCD for period
    unsigned short i = 0;
    unsigned long GCD = tasks[0]->period;
    for (i = 1; i < numTasks; ++i) {
        GCD = findGCD(GCD, tasks[i]->period);
    }

    TimerSet(GCD);
    TimerOn();
    PWM_on();

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
