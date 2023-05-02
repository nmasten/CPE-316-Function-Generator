/*
 * dac.h
 *
 *  Created on: Apr 24, 2023
 *      Author: noahmasten
 */

#ifndef SRC_DAC_H_
#include "main.h"
#define SRC_DAC_H_
#define DAC_HEADER 0x3000 // Calculated with MCP4922 data sheet
#define DAC_MASK 0xFFF // ensures voltage won't exceed 3.3 V
#define VDD 3300 // 3.3 V
#define SCALAR 1.245 // Voltage conversion factor

void DAC_Init(void);
void DAC_write(uint16_t data);
uint32_t DAC_volt_conv(uint32_t voltage);

#endif /* SRC_DAC_H_ */
