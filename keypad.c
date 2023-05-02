/*
 * keypad.c
 *
 *  Created on: Apr 12, 2023
 *      Author: Noah Masten, Cole Costa
 */

#include "main.h"
#include "keypad.h"

/*
 * CONFIGURES KEYPAD PINS FOR GPIO
 * PC4 - PC7 FOR ROWS
 * PB4 - PB7 FOR COLS
 */
void Keypad_Init(void) {

	// Enable GPIOB and GPIOC clocks
	RCC->AHB2ENR   |=  (RCC_AHB2ENR_GPIOBEN);
	RCC->AHB2ENR   |=  (RCC_AHB2ENR_GPIOCEN);

	// Configure rows for input
	ROW_PORT->MODER &= ~(GPIO_MODER_MODE4 | GPIO_MODER_MODE5 | GPIO_MODER_MODE6 | GPIO_MODER_MODE7);

	// Configure cols for output
	COL_PORT->MODER &= ~(GPIO_MODER_MODE4 | GPIO_MODER_MODE5 | GPIO_MODER_MODE6 | GPIO_MODER_MODE7);
	COL_PORT->MODER |= (GPIO_MODER_MODE4_0 | GPIO_MODER_MODE5_0 | GPIO_MODER_MODE6_0 | GPIO_MODER_MODE7_0);

	// Pull down for rows
	ROW_PORT->PUPDR &= ~(GPIO_PUPDR_PUPD4 | GPIO_PUPDR_PUPD5 | GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD7);
	ROW_PORT->PUPDR |= (GPIO_PUPDR_PUPD4_1 | GPIO_PUPDR_PUPD5_1 | GPIO_PUPDR_PUPD6_1 | GPIO_PUPDR_PUPD7_1);

	// No PUPD for cols
	COL_PORT->PUPDR &= ~(GPIO_PUPDR_PUPD4 | GPIO_PUPDR_PUPD5 | GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD7);

	// No push-pull for cols
	COL_PORT->OTYPER &= ~(GPIO_OTYPER_OT4 | GPIO_OTYPER_OT5 | GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7);

	// Low speed
	COL_PORT->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED4 | GPIO_OSPEEDR_OSPEED5 | GPIO_OSPEEDR_OSPEED6 | GPIO_OSPEEDR_OSPEED7);

	// Turn on cols
	COL_PORT->BSRR = (GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);

}

/* RETURNS THE INTEGER VALUE OF THE KEY PRESSED BY THE USER */
int32_t getKey(void) {
	int rows = (ROW_PORT->IDR & ROW_MASK); // get current state of row pins
	if (rows == 0) { // No key is pressed
		return NO_KEY_PRESS;
	} else { // Iterate over columns and rows to find the exact key pressed
		int row_num = 0;
		int col_num = 0;
		COL_PORT->BRR = (GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7); // turn cols off
		for (int col = GPIO_PIN_4; col <= GPIO_PIN_7; col <<= 1) { // Start with PIN 1, iterate through PIN 4
			COL_PORT->BSRR = col;
			int checkRows = (ROW_PORT->IDR & ROW_MASK); // See if any of the rows in this column are pressed
			if (checkRows != 0) { // If a row is pressed, figure out which row
				for (int row = GPIO_PIN_4; row <= GPIO_PIN_7; row <<= 1) {
					if (row==checkRows) { // If current row corresponds to the row pressed, return the keypress
						COL_PORT->BSRR = (GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7); // reset cols to high
						return keypad_matrix[row_num][col_num]; // return keypress
					}
					row_num++; // increment row
				}
			}
			col_num++; // increment col
			COL_PORT->BRR =  (GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7); // turn cols back off
		}
	}
	COL_PORT->BSRR = (GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7); // reset cols to high
	return NO_KEY_PRESS; // if keypress is missed

}




