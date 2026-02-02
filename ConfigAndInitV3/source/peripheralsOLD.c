/**
 * Handles peripherals
 *
 * @authors Josefine Nyholm, ... , ...
 *
 * @cite https://community.st.com/ysqtg83639/attachments/ysqtg83639/stm32-mcu-products-forum/65216/1/STM32-L476-ProgramReference-RM0351.pdf
 * @cite https://www.infineon.com/evaluation-board/DC-MOTORCONTR-BTN8982
 * @cite https://assunmotor.com/online-shop/encoder/am-en1611s003-series-electromagnetic
 */

#include "peripherals.h"
#include "stm32l4xx.h"

#define ENCODER_RESOLUTION         512							        // The number of pulses per revolution
#define ENCODER_COUNTER_RESOLUTION (ENCODER_RESOLUTION * 4) // Number of counts per revolution

#define ENCODER_MAX_WORKING_FREQUENCY 360000
#define ENCODER_RPM_MAX (ENCODER_MAX_WORKING_FREQUENCY / ENCODER_COUNTER_RESOLUTION) * 60
//#define MOTOR_RPM_MIN
#define MOTOR_MAX_FREQUENCY 30000

int16_t encoder;			// Global variable, can be used for debugging purposes

uint16_t counterPreviousTIM1   = 0;
uint32_t milliSecondsPrevious  = 0;

/* 
 *Enables both half-bridges to drive the motor 
 */
void Peripheral_GPIO_EnableMotor(void)
{	
	// GPIO - PA5, PA6
	
	GPIOA->BSRR = GPIO_PIN_5; // (Section 8.4.7) (PA5 - GPIO_output)
	GPIOA->BSRR = GPIO_PIN_6; // (Section 8.4.7) (PA6 - GPIO_output)
}

/* 
 * Disables both half-bridges to stop the motor 
 */
void Peripheral_GPIO_DisableMotor(void)
{
	// GPIO - PA5, PA6
	
	GPIOA->BSRR = (GPIO_PIN_5 << 16); // (Section 8.4.7) (PA5 - GPIO_output)
	GPIOA->BSRR = (GPIO_PIN_6 << 16); // (Section 8.4.7) (PA6 - GPIO_output)
}

/** 
 * Drives the motor in both directions 
 * 
 * @param[in] dutyCycle - The control value for the dutyCycle
 */
void Peripheral_PWM_ActuateMotor(int32_t dutyCycle) // TODO: Validate maximum RPM, handle counter overflows, make sure the algorithm results in a smooth signal
{
	// PB4 - TIM3_CH1
	// PA7 - TIM3_CH2
	// PWM mode 1, no prescaler
	
	if(dutyCycle >= 0)
	{
		TIM3->CCR1 = (uint16_t)((dutyCycle >> 19) & 0x7FF); // ARR = 2047 => 0x7FF(2047+1) ticks per period According to CubeMX settings for TIM3)
																												// ARR = 11 bits => CCR can only be 11 bits => shift dutyCycle 19 bits
		TIM3->CCR2 = 0;
	}
	else
	{
		TIM3->CCR1 = 0;
		TIM3->CCR2 = (uint16_t)((-dutyCycle >> 19) & 0x7FF); // ARR = 2047 => 0x7FF(2047+1) ticks per period According to CubeMX settings for TIM3)
																												 // ARR = 11 bits => CCR can only be 11 bits => shift dutyCycle 19 bits
	}
	
	/*
	// ARR - Handles PWM frequency
	uint32_t timerTicks = ((TIM3->ARR) + 1); // ARR Auto-Reload Register (Sections 31.3.1) (31.3.9)
	uint32_t captureCompare1 = TIM3->CCR1;
	uint32_t captureCompare2 = TIM3->CCR2;
 
	
	if(timerTicks > captureCompare1) // HIGH
		
	else
	
	// CCRx - Handles PWM duty cycle
	
	TIM3->CCR1 = 0;
	TIM3->CCR2 = 0;
	
	
	uint32_t dutyCycleChannel1 = (TIM3->CCR1 * 100) / frequency;
	uint32_t dutyCycleChannel2 = (TIM3->CCR1 * 100) / frequency;
	
	if(vel > ENCODER_RPM_MAX)
		vel = ENCODER_RPM_MAX;
		
	uint32_t dutyCycle = (du)
	
	TIM3->CCR1 = ducyCycle * (frequency)
	
	frequencyPWM = TIM3_CLOCK_FREQUENCY / ((TIM3->ARR) + 1)
	
	if(timerTicks >)
	*/
}

/**
 * Reads the encoder value and calculates the current velocity in RPM 
 *
 * @param[in] ms - The run tume in milli seconds
 */
int32_t Peripheral_Encoder_CalculateVelocity(uint32_t ms) // TODO: Handle counter wrap-around during milliSecondsDifference
{
	//PA9 - TIM1_CH2
	//PA8 - TIM1_CH1

	// Access adress C-code:
	// Memory adress(TIM1): 0x4001 2C00 (Section 2.2.2)
	// Offset (TIM1_CNT): 0x24 (Section 30.4.10)
	// TIM1_SMCR Momory adress: 0x4001 2C24
	
	// Access adress CMSIS-style:
	// TIM1_CNT

	int32_t velocityRPM;
	
	// Read the encoder(counter) value
	uint16_t counter = (uint16_t)(TIM1->CNT & 0xFFFF);
	
	uint16_t counterDifference      = counter - counterPreviousTIM1;
	uint32_t milliSecondsDifference = ms - milliSecondsPrevious;
	
	if(counterDifference == 0 || milliSecondsDifference == 0)
		return 0;
	
	velocityRPM = (int32_t)(((int64_t)counterDifference * 60000) / (ENCODER_COUNTER_RESOLUTION * milliSecondsDifference));
	
	if(counterPreviousTIM1 > counter) // Handles the reverse case
		velocityRPM = -1 * velocityRPM;
	
	counterPreviousTIM1      = counter;
	milliSecondsPrevious     = ms;
	
	return velocityRPM;
}
