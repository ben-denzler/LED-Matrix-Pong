# LED Matrix Pong
 
 Author: [Benjamin Denzler](https://github.com/ben-denzler)
 
 ## Project Description
 
 This is a fully-functioning Pong game running on a microcontroller with an LED matrix! The entire game is written in C, and I programmed an [ATMEGA1284P](https://www.microchip.com/wwwproducts/en/ATmega1284p) microcontroller using [UCR's AVR Toolchain](https://github.com/jmcda001/UCRCS120B_AVRTools). The microcontroller takes input from several buttons and a potentiometer, and outputs to an 8x8 LED matrix and stand-alone LEDs.
 
 The game has the following features:
 * Complete ball physics: paddle center bounces straight, paddle edges bounce at an angle
 * Changing ball speeds depending on where the ball is hit
 * Single-player mode with a simple AI, player uses buttons
 * Two-player mode that disables the AI, second player uses a potentiometer
 * Seamless switching between single and two-player modes
 * Score counter for each player (LEDs)
 * Winning conditions: first to three wins, and the game is reset
 * Reset button to clear the ball's position and proceed to next round
 
 The following hardware was used for this project:
 * [Digilent Solderless Breadboard](https://www.digikey.com/en/products/detail/digilent-inc/340-002-1/9556131)
 * [ATMEGA1284P Microcontroller](https://www.microchip.com/wwwproducts/en/ATmega1284p)
 * [Atmel-ICE AVR Programmer](https://www.digikey.com/en/products/detail/microchip-technology/ATATMEL-ICE-PCBA/4753383)
 * 8x8 LED Matrix
 * [50K Potentiometer](https://www.digikey.com/en/products/detail/tt-electronics-bi/P160KN2-0EC15B50K/5957458)
 * Miscellaneous wires, LEDs and resistors
