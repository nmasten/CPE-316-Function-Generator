/* main.c */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "keypad.h"
#include "dac.h"
#include <math.h>
#include <stdlib.h>

#define SINE 6 // Sine wave keypress
#define TRI 7 // Tri wave keypress
#define SAW 8 // Saw wave keypress
#define SQUARE 9 // Square wave keypress
#define CLK_SPEED 20000000 // 20 MHz clock
#define V_PEAK 3000 // peak to peak voltage, 3.0 V
#define V_LOW 0 // low voltage, 0 V
#define DEBOUNCE 4000 // debounce delay
#define V_CHANGE 26 // Voltage change per increment
#define NUM_WRITES 116 // number of writes per waveform period
#define PI 3.141592

void SystemClock_Config(void);
void change_timers(int8_t new_freq, int8_t new_duty);
void construct_waveform(void);

int8_t wave = SQUARE, freq = 1, duty = 50; // initial waveform: square wave, 100 Hz, 50% duty cycle
uint32_t curr_volt = 0; // current voltage being written and displayed

typedef enum { // define FSM states
	DISPLAY, CHANGE_WAVE, CHANGE_FREQ, CHANGE_DUTY
} state_var_type;

uint16_t *wave_array; // stores all voltages within one period
uint32_t array_size = 116; // beginning array size, NUM_WRITES / freq
uint16_t count = 0; // current index of wave_array

int main(void) {
	HAL_Init();
	SystemClock_Config();
	Keypad_Init();
	DAC_Init();

	int8_t keyPress = NO_KEY_PRESS; // Default key press value

	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN; // enable TIM2 clock

	TIM2->CR1 = (TIM_CR1_ARPE); // Enable Auto Reload/Preload (ARR)

	change_timers(freq, duty); // Initialize timers with 100 Hz, 50% duty cycle

	// Enable interrupt flags
	TIM2->DIER |= (TIM_DIER_UIE); // ARR flag
	TIM2->DIER |= (TIM_DIER_CC1IE); // CCR1 flag

	// enable interrupts in NVIC
	NVIC->ISER[0] = (1 << (TIM2_IRQn & 0x1F));

	// Turn on timer
	TIM2->CR1 |= TIM_CR1_CEN;

	construct_waveform(); // create 100 Hz, 50% duty square wave
	__enable_irq();

	state_var_type state = DISPLAY; // default FSM state

	while (1) {

		// FSM
		switch (state) {

		// Default state, func gen should always display a waveform and look for next key press
		case DISPLAY:
			if ((keyPress = getKey()) != NO_KEY_PRESS) { // wait for key to be pressed
				// Switch states depending on the function of the key press
				if (keyPress >= 1 && keyPress <= 5) {
					state = CHANGE_FREQ;
				} else if (keyPress >= 6 && keyPress <= 9) {
					state = CHANGE_WAVE;
				} else if (keyPress == 0 || keyPress == STAR || keyPress == HASHTAG) {
					state = CHANGE_DUTY;
				}
			}
			// wait for user to release button
			while (getKey() != NO_KEY_PRESS);
			for (int i = 0; i < DEBOUNCE; i++); // debounce delay
			break;

		case CHANGE_WAVE:
			wave = keyPress;

			// recalc values of wave_array with new wave
			construct_waveform();
			state = DISPLAY; // look for next key press
			break;

		case CHANGE_FREQ:
			freq = keyPress;

			// recalc ARR and CCR1 with new freq
			change_timers(freq, duty);

			// recalc values of wave_array with new freq
			construct_waveform();
			state = DISPLAY; // look for next key press
			break;

		case CHANGE_DUTY:

			// Check if the key pressed is STAR, 0, or HASHTAG
			switch (keyPress) {
			case STAR:
				duty -= 10; // decrement duty cycle by 10%

				// Ensure duty cycle does not go below 10%
				if (duty < 10) {
					duty = 10;
				}
				break;

			case 0:
				duty = 50; // set duty cycle to 50%
				break;

			case HASHTAG: // increment duty cycle by 10%
				duty += 10;

				// Ensure duty cycle does not exceed 90%
				if (duty > 90) {
					duty = 90;
				}
				break;

			default: // In case of error, keep the same duty cycle
				break;
			}

			// recalc ARR and CCR1 based on new duty cycle
			change_timers(freq, duty);
			state = DISPLAY; // look for next key press
			break;

		// In case of error, look for next key press
		default:
			state = DISPLAY;
			break;
		}
	}
}

/* TIMER 2 ISR */
void TIM2_IRQHandler(void) {

	// Once count reaches TIM2->ARR
	if (TIM2->SR & TIM_SR_UIF) {
		curr_volt = V_LOW; // reset current voltage to 0
		count = 0; // reset wave_array index

		// If not a square wave, reset CCR1
		TIM2->CCR1 = (wave != SQUARE) ? TIM2->ARR / array_size : TIM2->CCR1;
		TIM2->SR &= ~(TIM_SR_UIF); // reset interrupt flag
	} else if (TIM2->SR & TIM_SR_CC1IF) {
		curr_volt = (wave != SQUARE) ? wave_array[count] : V_PEAK; // set current voltage to wave_array value
		count++; // increment wave_array index

		// If not a square wave, increment CCR1
		TIM2->CCR1 += (wave != SQUARE) ? (TIM2->ARR / array_size) : 0;
		TIM2->SR &= ~(TIM_SR_CC1IF); // reset interrupt flag
	}
	DAC_write(DAC_volt_conv(curr_volt));
}

/* RECALCULATES ARR AND CCR1 BASED ON FREQUENCY AND DUTY CYCLE */
void change_timers(int8_t new_freq, int8_t new_duty) {
	TIM2->ARR = (CLK_SPEED / (new_freq * 100))+100; // Calculate ARR based on Clock Speed and frequency

	// If square wave, set CCR1 to a constant value
	if (wave == SQUARE) {
		TIM2->CCR1 = ((TIM2->ARR * new_duty) / 100) - 1; // Calculate CRR based off of ARR and duty cycle
	}

	// Must be done any time a timer value is changed
	TIM2->EGR |= (TIM_EGR_UG); // Update interrupt event

}

/* CALCULATES THE VALUES OF WAVE_ARRAY BASED ON CURRENT WAVEFORM AND FREQUENCY */
void construct_waveform(void) {
	free(wave_array); // free the memory of current wave_array
	array_size = NUM_WRITES / freq; // calculate new wave_array size
	wave_array = (uint16_t*) malloc(sizeof(uint16_t) * array_size); // realloc wave_array with new size
	uint32_t val = 0; // initial value of tri and saw waves

	// Calculate wave_array based on current waveform
	switch (wave) {
	case SINE:
		for (uint32_t i = 0; i < array_size; i++) {
			// Standard sine wave equation, 1500 mV amplitude and DC bias
			wave_array[i] = 1500 * sin(2 * PI * ((float) i / array_size)) + 1500;
		}
		break;

	case SAW:
		for (uint32_t i = 0; i < array_size; i++) {
			wave_array[i] = val;
			val += V_CHANGE * freq; // increment val by slope of saw wave
		}
		break;

	case TRI:
		for (uint32_t i = 0; i < array_size; i++) {
			wave_array[i] = val;

			if (i < array_size / 2) {
				val += 2 * V_CHANGE * freq; // slope increments twice as fast

			// If half the period has been reached, decrement voltage instead
			} else {
				val -= 2 * V_CHANGE * freq;
			}
		}
		break;

	// In case of error, output a sine wave
	default:
		for (uint32_t i = 0; i < array_size; i++) {
			wave_array[i] = 1500 * sin(2 * PI * ((float) i / array_size)) + 1500; // upon error, create sine wave
		}
		break;

	}

}
/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1)
			!= HAL_OK) {
		Error_Handler();
	}

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 20;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
