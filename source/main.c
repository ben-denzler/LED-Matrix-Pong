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
unsigned char gameStart = 0;  // 0 = game reset/paused, 1 = game start
enum BallStatus_States { BS_Wait, BS_Right, BS_Left, BS_UpRight, BS_DownRight, BS_UpLeft, BS_DownLeft, BS_Miss };  // Possible movements for ball
unsigned char rows[8] = { 0x1C, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x1C };  // First, last are paddles
//              Rows:      1      2     3     4     5     6     7     8
unsigned char ballPattern = 0x08;  // Ball is initially 0x08
unsigned char ballRowIndex = 3;    // Ball is initially index 3 in rows[]

// Paddle variables
const unsigned char paddleTopPos = 0xE0;
const unsigned char paddleBotPos = 0x07;
#define rightPaddlePattern rows[0]
#define leftPaddlePattern rows[7]

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

// Resets row patterns to default
void resetRows() {
    rows[0] = 0x1C; rows[1] = 0x00; rows[2] = 0x00;
    rows[3] = 0x08; rows[4] = 0x00; rows[5] = 0x00;
    rows[6] = 0x00; rows[7] = 0x1C;
    ballPattern = 0x08;
    ballRowIndex = 3;
}

// Removes ball from screen
void clearBall() {
    rows[1] = 0x00; rows[2] = 0x00; rows[3] = 0x00; 
    rows[4] = 0x00; rows[5] = 0x00; rows[6] = 0x00;
}

// Possible paddle locations (6):
// Top: 0xE0
// 	    0x70
// 	    0x31
// 	    0x1C
// 	    0x0E
// Bot: 0x07

// Update the ball's status/direction
int updateBallStatus() {
    int newBallStatus = -1;
    switch (ballRowIndex) { // Check what side ball is on
        case 1:  // Right side
            switch (rightPaddlePattern) {   // Check where the right paddle is
                case 0xE0:  // Top
                    switch (ballPattern) { default: break; }
                case 0x70:
                    switch (ballPattern) { default: break; }
                case 0x31:
                    switch (ballPattern) { default: break; }
                case 0x1C:
                    switch (ballPattern) {
                        case 0x08: newBallStatus = BS_Left; break;
                        default: break;
                    }
                case 0x0E:
                    switch (ballPattern) { default: break; }
                case 0x07:  // Bottom
                    switch (ballPattern) { default: break; }
                default: break;
            }
            break;
        case 6:  // Left side
            switch (leftPaddlePattern) {    // Check where the left paddle is
                case 0xE0:  // Top
                    switch (ballPattern) { default: break; }
                case 0x70:
                    switch (ballPattern) { default: break; }
                case 0x31:
                    switch (ballPattern) { default: break; }
                case 0x1C:
                    switch (ballPattern) {
                        case 0x08: newBallStatus = BS_Right; break;
                        default: break;
                    }
                case 0x0E:
                    switch (ballPattern) { default: break; }
                case 0x07:  // Bottom
                    switch (ballPattern) { default: break; }
                default: break;
            }
            break;
        default: break;
    }
    return newBallStatus;
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

// Handles ball physics
int BallStatus_Tick(int state) {
    unsigned char r = rand() % 10;  // Decides initial ball direction
    displayChar(ballRowIndex);  // DEBUGGING

    switch (state) {    // Transitions
        case BS_Wait:
            if (!gameStart) { state = BS_Wait; }
            else if (r >= 5) { state = BS_Right; }
            else if (r < 5) { state = BS_Left; }
            break;

        case BS_Right:
            if (!gameStart) {
                resetRows();
                state = BS_Wait;
            }
            if (ballRowIndex == 1) { state = updateBallStatus(); }
            else { state = BS_Right; }
            break; 

        case BS_Left:
            if (!gameStart) {
                resetRows();
                state = BS_Wait;
            }
            else if (ballRowIndex == 6) { state = updateBallStatus(); }
            else { state = BS_Left; }
            break;

        default: state = BS_Wait; break;
    }
    switch (state) {    // Actions
        case BS_Wait: break;

        case BS_Right:
            rows[ballRowIndex] = 0x00;
            rows[--ballRowIndex] = ballPattern;
            break;

        case BS_Left:
            rows[ballRowIndex] = 0x00;
            rows[++ballRowIndex] = ballPattern;
            break;

        default: break;
    }
    displayChar(ballRowIndex);  // DEBUGGING
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
    // DDRB = 0x40; PORTB = 0xBF;  // PB6 is output (PWM), rest is input
    DDRB = 0xFF; PORTB = 0x00;  // PORTB is output (PWM, debugging)
    DDRC = 0xFF; PORTC = 0x00;  // Pin C is output (matrix cols, GROUND)
    DDRD = 0xFF; PORTD = 0x00;  // Pin D is output (matrix rows, OUTPUT)

    // Array of tasks
    static task task1, task2, task3, task4;
    task* tasks[] = { &task1, &task2, &task3, &task4 };
    const unsigned short numTasks = (sizeof(tasks) / sizeof(task*));

    const char start = -1;
    unsigned short j = 0;

    // Task 1 (RightPaddleInput_Tick)
    tasks[j]->state = start;
    tasks[j]->period = 10;
    tasks[j]->elapsedTime = tasks[j]->period;
    tasks[j]->TickFct = &RightPaddleInput_Tick;
    ++j;

    // Task 2 (BallStatus_Tick)
    tasks[j]->state = start;
    tasks[j]->period = 500;
    tasks[j]->elapsedTime = tasks[j]->period;
    tasks[j]->TickFct = &BallStatus_Tick;
    ++j;

    // Task 3 (Output_Tick)
    tasks[j]->state = start;
    tasks[j]->period = 1;
    tasks[j]->elapsedTime = tasks[j]->period;
    tasks[j]->TickFct = &Output_Tick;
    ++j;

    // Task 4 (StartReset_Tick)
    tasks[j]->state = start;
    tasks[j]->period = 20;
    tasks[j]->elapsedTime = tasks[j]->period;
    tasks[j]->TickFct = &StartReset_Tick;
    ++j;

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
