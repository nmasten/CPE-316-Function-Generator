/*
 * keypad.h
 *
 *  Created on: Apr 12, 2023
 *      Author: Noah Masten, Cole Costa
 */

#include "main.h"
#ifndef SRC_KEYPAD_H_
#define SRC_KEYPAD_H_

#define ROW_PORT GPIOC
#define COL_PORT GPIOB
#define ROW_MASK (GPIO_IDR_ID4 | GPIO_IDR_ID5 | GPIO_IDR_ID6 | GPIO_IDR_ID7) // mask that combines the four row pins
#define LETTER_A 10
#define LETTER_B 11
#define LETTER_C 12
#define LETTER_D 13
#define STAR 14
#define HASHTAG 15
#define NO_KEY_PRESS -1

int32_t const keypad_matrix[4][4] = {{1, 2, 3, LETTER_A},
									{4, 5, 6, LETTER_B},
									{7, 8, 9, LETTER_C},
									{STAR ,0, HASHTAG, LETTER_D}};

void Keypad_Init(void);
int32_t getKey(void);

#endif




