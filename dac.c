/*
 * dac.c
 *
 *  Created on: Apr 24, 2023
 *      Author: noahmasten
 */

#include "dac.h"
#include "main.h"

/* CONFIRURE PA4, PA5, PA7 FOR GPIO AND SPI
 * PA4 FOR CHIP SELECT (CS/NSS)
 * PA5 FOR SERIAL CLOCK (SCK)
 * PA7 FOR PERIPHERAL IN/CONTROLLER OUT
 */
void DAC_Init(void) {

	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; // Enable GPIOA clock
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN; // Enable SPI1 clock

	// clear GPIO mode
	GPIOA->MODER &= ~(GPIO_MODER_MODE4|
					  GPIO_MODER_MODE5|
					  GPIO_MODER_MODE7);

	// set GPIO mode to alternate function
	GPIOA->MODER |= (GPIO_MODER_MODE4_1)|
					(GPIO_MODER_MODE5_1)|
					(GPIO_MODER_MODE7_1);

	// set alternate function for GPIO to SPI
	GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL4 |
					   GPIO_AFRL_AFSEL5 |
					   GPIO_AFRL_AFSEL7);

	GPIOA->AFR[0] |= (5<< GPIO_AFRL_AFSEL4_Pos)|
					 (5<< GPIO_AFRL_AFSEL5_Pos)|
					 (5<< GPIO_AFRL_AFSEL7_Pos);

	// OTYPER -> push pull
	GPIOA->OTYPER &= ~(GPIO_OTYPER_OT4
			| GPIO_OTYPER_OT5
			| GPIO_OTYPER_OT7);

	// OSPEED -> low speed
	GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED4
			| GPIO_OSPEEDR_OSPEED5
			| GPIO_OSPEEDR_OSPEED7);

	// PUPDR -> no pull up pull down
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD4
			| GPIO_PUPDR_PUPD5
			| GPIO_PUPDR_PUPD7);

	// initialize SPI
	SPI1->CR1 = (SPI_CR1_MSTR);

	SPI1->CR2 = (SPI_CR2_DS)|
				  SPI_CR2_NSSP|
				  SPI_CR2_ERRIE;

	// enable SPI
	SPI1->CR1 |= (SPI_CR1_SPE);

}

/* WRITE VOLTAGE TO THE DAC */
void DAC_write(uint16_t data) {
	SPI1->DR = data; // write voltage to SPI data register
}

/* CONVERTS VOLTAGE AND PREPENDS DAC_HEADER SO VOLTAGE CAN BE WRITTEN TO DAC */
uint32_t DAC_volt_conv(uint32_t voltage) {

	uint16_t dacData = 0;
	voltage *= SCALAR; // Adjust voltage using conversion factor

	// check if voltage exceeds max (VDD)
	if (voltage > VDD) {
		dacData = VDD & DAC_MASK; // max allowed voltage is 3.3 V
	} else {
		dacData = voltage & DAC_MASK; // else, keep current voltage
	}

	return dacData | DAC_HEADER; // return value with header of 0x3
}
