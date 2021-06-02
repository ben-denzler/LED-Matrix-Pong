/* Author: Benjamin Denzler
 * Lab Section: 21
 * Assignment: Lab #11
 * Exercise Description: Pong (Advancement 2)
 *
 * I acknowledge all content contained herein, excluding template or example
 * code is my own original work.
 *
 *  Demo Link: https://youtu.be/vE6awcY7LqY
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

// PA7 = Right paddle up
// PA1 = Right paddle down
// PA2 = Start/reset

// Game info
const unsigned char numRows = 8;
unsigned char gameStart = 0;        // 0 = game reset/paused, 1 = game start
unsigned long ballSpeed = 300;      // Ball initially moves @ 300ms period
unsigned char ballPattern = 0x08;   // Ball is initially 0x08
unsigned char ballRowIndex = 3;     // Ball is initially index 3 in rows[]
unsigned char player2Enable = 0;    // Is player 2 enabled?
unsigned char player1Points = 0;    // Player 1's points
unsigned char player2Points = 0;    // Player 2's points

enum BallStatus_States { BS_Wait, BS_Right, BS_Left, 
                        BS_UpRight, BS_DownRight, BS_UpLeft, 
                        BS_DownLeft, BS_MissRight, BS_MissLeft } currBallStatus;  // Possible movements for ball

unsigned char rows[8] = { 0x1C, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x1C };  // First, last are paddles
//     Rows:    (Top)      1      2     3     4     5     6     7     8      (Bot)

// Paddle variables
const unsigned char paddleTopPos = 0xE0;
const unsigned char paddleBotPos = 0x07;
#define rightPaddlePattern rows[0]
#define leftPaddlePattern rows[7]

// Set flags to use A/D converter
void ADC_init() { ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE); }

// Use potentiometer to set P2 paddle
void setPlayer2Paddle() {
    unsigned short x = ADC;
    if (x <= 220) { leftPaddlePattern = 0x07; }
    else if (x <= 378) { leftPaddlePattern = 0x0E; }
    else if (x <= 536) { leftPaddlePattern = 0x1C; }
    else if (x <= 694) { leftPaddlePattern = 0x38; }
    else if (x <= 852) { leftPaddlePattern = 0x70; }
    else { leftPaddlePattern = 0xE0; }
}

// Resets row patterns to default
void softReset() {
    if (!player2Enable) { leftPaddlePattern = 0x1C; }
    rightPaddlePattern = 0x1C;
    rows[1] = 0x00; rows[2] = 0x00; rows[3] = 0x08; 
    rows[4] = 0x00; rows[5] = 0x00; rows[6] = 0x00; 
    ballSpeed = 300;
    ballPattern = 0x08;
    ballRowIndex = 3;
    currBallStatus = BS_Wait;
    if ((player1Points >= 3) || (player2Points >= 3)) {
        player1Points = 0;
        player2Points = 0;
    }
}

// Removes ball from screen
void clearBall() {
    rows[1] = 0x00; rows[2] = 0x00; rows[3] = 0x00; 
    rows[4] = 0x00; rows[5] = 0x00; rows[6] = 0x00;
}

// Speed up ball
void speedUpBall() { if (ballSpeed >= 200) { ballSpeed -= 50; } }

// Slow down ball
void slowDownBall() { if (ballSpeed <= 350) { ballSpeed += 50; } }

// Update LEDs for player points
void setPointsLEDs() {
    switch (player1Points) {
        case 1: PORTB = SetBit(PORTB, 2, 1); break;
        case 2: PORTB = SetBit(PORTB, 3, 1); break;
        case 3: PORTB = SetBit(PORTB, 4, 1); break;
        default:
            PORTB = SetBit(PORTB, 2, 0);
            PORTB = SetBit(PORTB, 3, 0);
            PORTB = SetBit(PORTB, 4, 0);
            break;
    }
    switch (player2Points) {
        case 1: PORTA = SetBit(PORTA, 4, 1); break;
        case 2: PORTA = SetBit(PORTA, 5, 1); break;
        case 3: PORTA = SetBit(PORTA, 6, 1); break;
        default:
            PORTA = SetBit(PORTA, 4, 0);
            PORTA = SetBit(PORTA, 5, 0);
            PORTA = SetBit(PORTA, 6, 0);
            break;
    }
}

// Update the ball's status/direction
int updateBallStatus() {
    int newBallStatus = -1;
    switch (ballRowIndex) {   // Check what side ball is on
        case 1:   // Right side
            switch (rightPaddlePattern) {   // Check where the right paddle is
                case 0xE0:   // Top
                    switch (ballPattern) {   // Check where the ball is
                        case 0x80: newBallStatus = BS_DownLeft; speedUpBall(); break;
                        case 0x40: newBallStatus = BS_Left; slowDownBall(); break;
                        case 0x20: newBallStatus = BS_DownLeft; speedUpBall(); break;
                        default: newBallStatus = BS_MissRight; break;
                    }
                    break;

                case 0x70:
                    switch (ballPattern) {
                        case 0x40: newBallStatus = BS_UpLeft; speedUpBall(); break;
                        case 0x20: newBallStatus = BS_Left; slowDownBall(); break;
                        case 0x10: newBallStatus = BS_DownLeft; speedUpBall(); break;
                        default: newBallStatus = BS_MissRight; break;
                    }
                    break;

                case 0x38:
                    set_PWM(261.63);
                    switch (ballPattern) {
                        case 0x20: newBallStatus = BS_UpLeft; speedUpBall(); break;
                        case 0x10: newBallStatus = BS_Left; slowDownBall(); break;
                        case 0x08: newBallStatus = BS_DownLeft; speedUpBall(); break;
                        default: newBallStatus = BS_MissRight; break;
                    }
                    break;

                case 0x1C:   // Mid
                    switch (ballPattern) {
                        case 0x10: newBallStatus = BS_UpLeft; speedUpBall(); break;
                        case 0x08: newBallStatus = BS_Left; slowDownBall(); break;
                        case 0x04: newBallStatus = BS_DownLeft; speedUpBall(); break;
                        default: newBallStatus = BS_MissRight; break;
                    }
                    break;

                case 0x0E:
                    switch (ballPattern) {
                        case 0x08: newBallStatus = BS_UpLeft; speedUpBall(); break;
                        case 0x04: newBallStatus = BS_Left; slowDownBall(); break;
                        case 0x02: newBallStatus = BS_DownLeft; speedUpBall(); break;
                        default: newBallStatus = BS_MissRight; break;
                    }
                    break;

                case 0x07:   // Bottom
                    switch (ballPattern) {
                        case 0x04: newBallStatus = BS_UpLeft; speedUpBall(); break;
                        case 0x02: newBallStatus = BS_Left; slowDownBall(); break;
                        case 0x01: newBallStatus = BS_UpLeft; speedUpBall(); break;
                        default: newBallStatus = BS_MissRight; break;
                    }
                    break;

                default: break;
            }
            break;

        case 6:  // Left side
            switch (leftPaddlePattern) {    // Check where the left paddle is
                case 0xE0:   // Top
                    switch (ballPattern) {
                        case 0x80: newBallStatus = BS_DownRight; speedUpBall(); break;
                        case 0x40: newBallStatus = BS_Right; slowDownBall(); break;
                        case 0x20: newBallStatus = BS_DownRight; speedUpBall(); break;
                        default: newBallStatus = BS_MissLeft; break;
                    }
                    break;

                case 0x70:
                    switch (ballPattern) {
                        case 0x40: newBallStatus = BS_UpRight; speedUpBall(); break;
                        case 0x20: newBallStatus = BS_Right; slowDownBall(); break;
                        case 0x10: newBallStatus = BS_DownRight; speedUpBall(); break;
                        default: newBallStatus = BS_MissLeft; break;
                    }
                    break;

                case 0x38:
                    switch (ballPattern) {
                        case 0x20: newBallStatus = BS_UpRight; speedUpBall(); break;
                        case 0x10: newBallStatus = BS_Right; slowDownBall(); break;
                        case 0x08: newBallStatus = BS_DownRight; speedUpBall(); break;
                        default: newBallStatus = BS_MissLeft; break;
                    }
                    break;

                case 0x1C:   // Mid
                    switch (ballPattern) {
                        case 0x10: newBallStatus = BS_UpRight; speedUpBall(); break;
                        case 0x08: newBallStatus = BS_Right; slowDownBall(); break;
                        case 0x04: newBallStatus = BS_DownRight; speedUpBall(); break;
                        default: newBallStatus = BS_MissLeft; break;
                    }
                    break;

                case 0x0E:
                    switch (ballPattern) {
                        case 0x08: newBallStatus = BS_UpRight; speedUpBall(); break;
                        case 0x04: newBallStatus = BS_Right; slowDownBall(); break;
                        case 0x02: newBallStatus = BS_DownRight; speedUpBall(); break;
                        default: newBallStatus = BS_MissLeft; break;
                    }
                    break;

                case 0x07:   // Bottom
                    switch (ballPattern) {
                        case 0x04: newBallStatus = BS_UpRight; speedUpBall(); break;
                        case 0x02: newBallStatus = BS_Right; slowDownBall(); break;
                        case 0x01: newBallStatus = BS_UpRight; speedUpBall(); break;
                        default: newBallStatus = BS_MissLeft; break;
                    }
                    break;

                default: break;
            }
            break;
        default: break;
    }
    return newBallStatus;
}

// Update gameStart and reset if needed
enum StartReset_States { SR_Wait, SR_Update };
int StartReset_Tick(int state) {
    unsigned char tmpPA2 = (~PINA) & 0x04;  // PA2 = Start/reset

    switch (state) {    // Transitions
        case SR_Wait:
            if (tmpPA2) { 
                state = SR_Update; 
                gameStart = ~gameStart;
                if (!gameStart) { softReset(); }
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
    unsigned char tmpPA7 = (~PINA) & 0x80;  // PA7 = right paddle up
    unsigned char tmpPA1 = (~PINA) & 0x02;  // PA1 = right paddle down

    switch (state) {    // Transitions
        case RPI_Wait:
            if ((rightPaddlePattern != paddleTopPos) && tmpPA7) {
                rightPaddlePattern <<= 1;
                state = RPI_Up;
            }
            else if ((rightPaddlePattern != paddleBotPos) && tmpPA1) {
                rightPaddlePattern >>= 1;
                state = RPI_Down;
            }
            break;

        case RPI_Up:
            if (tmpPA7) { state = RPI_Up; }
            else if (!tmpPA7) { state = RPI_Wait; }
            break;

        case RPI_Down:
            if (tmpPA1) { state = RPI_Down; }
            else if (!tmpPA1) { state = RPI_Wait; }
            break;

        default: state = RPI_Wait; break;
    }
    return state;
}

// Update player2Enable when button is pressed
enum LeftPaddleScanInput_States { LPSI_Wait, LPSI_Update };
int LeftPaddleScanInput_Tick(int state) {
    unsigned char tmpPA3 = (~PINA) & 0x08;  // PA3 = player 2 join

    switch (state) {    // Transitions
        case LPSI_Wait: 
            if (tmpPA3) { 
                player2Enable = ~player2Enable;
                state = LPSI_Update; 
            }
            else { state = LPSI_Wait; }
            break;

        case LPSI_Update:
            if (tmpPA3) { state = LPSI_Update; }
            else { state = LPSI_Wait; }
            break;

        default: state = LPSI_Wait; break;
    }
    return state;
}

// Get input, update left paddle position
enum LeftPaddleInput_States { LPI_Wait, LPI_Update };
int LeftPaddleInput_Tick(int state) {
    switch (state) {    // Transitions
        case LPI_Wait:
            if (!player2Enable) { state = LPI_Wait; }
            else { state = LPI_Update; }
            break;

        case LPI_Update:
            if (player2Enable && (currBallStatus != BS_MissLeft)) {
                setPlayer2Paddle();
                state = LPI_Update;
            }
            else { state = LPI_Wait; }
            break;

        default: state = LPI_Wait; break;
    }
    return state;
}

// Update LEDs for # of players
enum UpdateNumPlayers_States { UNP_Update };
int UpdateNumPlayers_Tick(int state) {
    switch (state) {    // Transitions
        case UNP_Update:
            if (player2Enable) { 
                PORTB = SetBit(PORTB, 0, 1); 
                PORTB = SetBit(PORTB, 1, 1); 
            }
            else {
                PORTB = SetBit(PORTB, 0, 1); 
                PORTB = SetBit(PORTB, 1, 0); 
            }
            break;
        default: state = UNP_Update; break;
    }
    return state;
}

enum UpdatePlayerScores_States { UPS_Wait, UPS_Update };
int UpdatePlayerScores_Tick(int state) {
    switch (state) {    // Transitions
        case UPS_Wait:
            if ((currBallStatus == BS_MissRight) && (player2Points < 3)) {
                ++player2Points;
                state = UPS_Update;
            }
            else if ((currBallStatus == BS_MissLeft) && (player1Points < 3)) {
                ++player1Points;
                state = UPS_Update;
            }
            else { state = UPS_Wait; }
            break;

        case UPS_Update:
            if ((currBallStatus == BS_MissRight) || (currBallStatus == BS_MissLeft)) { state = UPS_Update; }
            else { state = UPS_Wait; }
            break;

        default: state = UPS_Wait; break;
    }
    switch (state) {    // Actions
        case UPS_Wait: break;
        case UPS_Update: setPointsLEDs(); break;
        default: break;
    }
    return state;
}

// Handles ball physics
int BallStatus_Tick(int state) {
    unsigned char r = rand() % 10;  // Decides initial ball direction

    switch (state) {    // Transitions
        case BS_Wait:
            if (!gameStart) { state = BS_Wait; }
            else if (r >= 5) { state = BS_Right; }
            else if (r < 5) { state = BS_Left; }
            break;

        case BS_Right:
            if (!gameStart) {
                softReset();
                state = BS_Wait;
            }
            else if (ballRowIndex == 1) { state = updateBallStatus(); }
            else { state = BS_Right; }
            break; 

        case BS_Left:
            if (!gameStart) {
                softReset();
                state = BS_Wait;
            }
            else if (ballRowIndex == 6) { state = updateBallStatus(); }
            else { state = BS_Left; }
            break;

        case BS_UpLeft:
            if (!gameStart) {
                softReset();
                state = BS_Wait;
            }
            else if ((ballPattern == 0x80) && (ballRowIndex != 6)) { state = BS_DownLeft; }
            else if (ballRowIndex == 6) { state = updateBallStatus(); }
            else { state = BS_UpLeft; }
            break;

        case BS_DownLeft:
            if (!gameStart) {
                softReset();
                state = BS_Wait;
            }
            else if ((ballPattern == 0x01) && (ballRowIndex != 6)) { state = BS_UpLeft; }
            else if (ballRowIndex == 6) { state = updateBallStatus(); }
            else { state = BS_DownLeft; }
            break;

        case BS_UpRight:
            if (!gameStart) {
                softReset();
                state = BS_Wait;
            }
            else if ((ballPattern == 0x80) && (ballRowIndex != 1)) { state = BS_DownRight; }
            else if (ballRowIndex == 1) { state = updateBallStatus(); }
            else { state = BS_UpRight; }
            break;

        case BS_DownRight:
            if (!gameStart) {
                softReset();
                state = BS_Wait;
            }
            else if ((ballPattern == 0x01) && (ballRowIndex != 1)) { state = BS_UpRight; }
            else if (ballRowIndex == 1) { state = updateBallStatus(); }
            else { state = BS_DownRight; }
            break;

        case BS_MissRight:
            if (!gameStart) { state = BS_MissRight; }
            else {
                softReset();
                gameStart = 0;
                state = BS_Wait;
            }
            break;

        case BS_MissLeft:
            if (!gameStart) { state = BS_MissLeft; }
            else {
                softReset();
                gameStart = 0;
                state = BS_Wait;
            }
            break;

        default: state = BS_Wait; break;
    }
    switch (state) {    // Actions
        case BS_Wait: 
            currBallStatus = BS_Wait; 
            setPointsLEDs();
            break;

        case BS_Right:
            currBallStatus = BS_Right;
            rows[ballRowIndex] = 0x00;
            rows[--ballRowIndex] = ballPattern;
            break;

        case BS_Left:
            currBallStatus = BS_Left;
            rows[ballRowIndex] = 0x00;
            rows[++ballRowIndex] = ballPattern;
            break;

        case BS_UpLeft:
            currBallStatus = BS_UpLeft;
            rows[ballRowIndex] = 0x00;
            ballPattern <<= 1;
            rows[++ballRowIndex] = ballPattern;
            break;

        case BS_DownLeft:
            currBallStatus = BS_DownLeft;
            rows[ballRowIndex] = 0x00;
            ballPattern >>= 1;
            rows[++ballRowIndex] = ballPattern;
            break;

        case BS_UpRight:
            currBallStatus = BS_UpRight;
            rows[ballRowIndex] = 0x00;
            ballPattern <<= 1;
            rows[--ballRowIndex] = ballPattern;
            break;

        case BS_DownRight:
            currBallStatus = BS_DownRight;
            rows[ballRowIndex] = 0x00;
            ballPattern >>= 1;
            rows[--ballRowIndex] = ballPattern;
            break;

        case BS_MissRight:
            currBallStatus = BS_MissRight;
            gameStart = 0;
            setPointsLEDs();
            clearBall();
            rightPaddlePattern = 0xFF;
            break;

        case BS_MissLeft:
            currBallStatus = BS_MissLeft;
            gameStart = 0;
            setPointsLEDs();
            clearBall();
            leftPaddlePattern = 0xFF;
            break;

        default: break;
    }
    return state;
}

// Simple AI to control left paddle
enum LeftPaddleAI_States { LPAI_Wait, LPAI_MoveUp, LPAI_MoveDown };
int LeftPaddleAI_Tick(int state) {
    unsigned char r = rand() % 10;   // Decides whether to follow ball

    switch (state) {    // Transitions
        case LPAI_Wait: 
            if ((r >= 2) && (currBallStatus == BS_UpLeft) && !player2Enable) { state = LPAI_MoveUp; }
            else if ((r >= 2) && (currBallStatus == BS_DownLeft) && !player2Enable) { state = LPAI_MoveDown; }
            else { state = LPAI_Wait; }
            break;

        case LPAI_MoveUp: state = LPAI_Wait; break;
        case LPAI_MoveDown: state = LPAI_Wait; break;
        default: state = LPAI_Wait; break;
    }
    switch (state) {    // Actions
        case LPAI_Wait: break;
        case LPAI_MoveUp: if (leftPaddlePattern != 0xE0) { leftPaddlePattern <<= 1; } break;
        case LPAI_MoveDown: if (leftPaddlePattern != 0x07) { leftPaddlePattern >>= 1; } break;
        default: break;
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
    DDRA = 0x70; PORTA = 0x8F;  // Pin A is input/output
    DDRB = 0xFF; PORTB = 0x00;  // PORTB is output (PWM etc.)
    DDRC = 0xFF; PORTC = 0x00;  // Pin C is output (matrix cols, GROUND)
    DDRD = 0xFF; PORTD = 0x00;  // Pin D is output (matrix rows, OUTPUT)

    // Array of tasks
    static task task1, task2, task3, task4, task5, task6, task7, task8, task9;
    task* tasks[] = { &task1, &task2, &task3, &task4, &task5, &task6, &task7, &task8, &task9 };
    const unsigned short numTasks = (sizeof(tasks) / sizeof(task*));

    const char start = -1;
    unsigned short j = 0;

    // Task 1 (RightPaddleInput_Tick)
    tasks[j]->state = start;
    tasks[j]->period = 10;
    tasks[j]->elapsedTime = tasks[j]->period;
    tasks[j]->TickFct = &RightPaddleInput_Tick;
    ++j;

    // Task 2 (LeftPaddleInput_Tick)
    tasks[j]->state = start;
    tasks[j]->period = 10;
    tasks[j]->elapsedTime = tasks[j]->period;
    tasks[j]->TickFct = &LeftPaddleInput_Tick;
    ++j;

    // Task 3 (LeftPaddleScanInput_Tick)
    tasks[j]->state = start;
    tasks[j]->period = 10;
    tasks[j]->elapsedTime = tasks[j]->period;
    tasks[j]->TickFct = &LeftPaddleScanInput_Tick;
    ++j;

    // Task 4 (UpdateNumPlayers_Tick)
    tasks[j]->state = start;
    tasks[j]->period = 10;
    tasks[j]->elapsedTime = tasks[j]->period;
    tasks[j]->TickFct = &UpdateNumPlayers_Tick;
    ++j;

    // Task 5 (BallStatus_Tick)
    tasks[j]->state = start;
    tasks[j]->period = ballSpeed;
    tasks[j]->elapsedTime = tasks[j]->period;
    tasks[j]->TickFct = &BallStatus_Tick;
    ++j;

    // Task 6 (LeftPaddleAI_Tick)
    tasks[j]->state = start;
    tasks[j]->period = 200;
    tasks[j]->elapsedTime = tasks[j]->period;
    tasks[j]->TickFct = &LeftPaddleAI_Tick;
    ++j;

    // Task 7 (Output_Tick)
    tasks[j]->state = start;
    tasks[j]->period = 1;
    tasks[j]->elapsedTime = tasks[j]->period;
    tasks[j]->TickFct = &Output_Tick;
    ++j;

    // Task 8 (StartReset_Tick)
    tasks[j]->state = start;
    tasks[j]->period = 20;
    tasks[j]->elapsedTime = tasks[j]->period;
    tasks[j]->TickFct = &StartReset_Tick;
    ++j;

    // Task 9 (UpdatePlayerScores_Tick)
    tasks[j]->state = start;
    tasks[j]->period = 10;
    tasks[j]->elapsedTime = tasks[j]->period;
    tasks[j]->TickFct = &UpdatePlayerScores_Tick;
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
    ADC_init();

    while (1) {
        for (i = 0; i < numTasks; ++i) {
            if (tasks[i]->TickFct == &BallStatus_Tick) { tasks[i]->period = ballSpeed; }   // Update ball speed
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
