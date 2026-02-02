/**
 * Handles peripherals
 *
 * @file peripherals.c
 *
 * @authors Josefine Nyholm, Nils Kiefer, Arunraghavendren Senthil Kumar
 *
 * @cite https://community.st.com/ysqtg83639/attachments/ysqtg83639/stm32-mcu-products-forum/65216/1/STM32-L476-ProgramReference-RM0351.pdf
 * @cite https://www.infineon.com/evaluation-board/DC-MOTORCONTR-BTN8982
 * @cite https://assunmotor.com/online-shop/encoder/am-en1611s003-series-electromagnetic
 */
 
#include "peripherals.h"
#include "stm32l4xx.h"
#include <stdint.h>

/* ----------------- Units & scaling ----------------- */

// Control input uses signed Q30: full scale = [-2^30, 2^30-1]
// Fixed-point is used here because the assignment forbids float usage.
#define CTRL_Q 30
#define CTRL_MAX ((int32_t)0x3FFFFFFF)
#define CTRL_MIN ((int32_t)0xC0000000)

/* ----------------- Config (tune in Watch) ----------------- */

// Encoder resolution (quadrature decoding => 4x)
#define ENCODER_PPR            512
#define ENCODER_COUNTS_PER_REV (ENCODER_PPR * 4)

static uint16_t counterPreviousTIM1  = 0;
static uint32_t milliSecondsPrevious = 0;

// Saturate controller input to the allowed Q30 range.
// Convert Q30 control value to timer counts in range [0, ARR].
static inline int32_t ctrl_to_counts(int32_t ctrl, uint32_t top) 
{
		//Clamping
    if (ctrl > CTRL_MAX) 
			ctrl = CTRL_MAX;
    else if (ctrl < CTRL_MIN) 
			ctrl = CTRL_MIN;
		
    // Handle CTRL_MIN specially to avoid overflow when negating.
    int32_t duty = (int32_t)(((int64_t)ctrl * (int64_t)top) >> CTRL_Q);

    // Clip to top
    if (duty > (int32_t)top - 1)
        duty = (int32_t)top - 1;

    // clip to min
    if (duty < -(int32_t)top + 1)
        duty = -(int32_t)top + 1;

    return duty;
}

/* ----------------- GPIO ----------------- */

/* 
 * Enables both half-bridges to drive the motor 
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

/* ----------------- PWM ----------------- */
/** 
 * Drives the motor in both directions 
 * 
 * @param[in] controlDutyCycle - The control value for the dutyCycle
 */
void Peripheral_PWM_ActuateMotor(int32_t controlDutyCycle) 
{
	// ARR is the timer period, so top = ARR + 1 counts.
	const uint32_t pwm_top = ((TIM3->ARR) + 1); // ARR Auto-Reload Register (Sections 31.3.1) (31.3.9);
	const int32_t dutyCycle = ctrl_to_counts(controlDutyCycle, pwm_top);

	// Direction is set by choosing which PWM channel is active.
	if(dutyCycle > 0) // Clockwise: use CCR2, keep CCR1 low.
	{
		TIM3->CCR1 = 0;
		TIM3->CCR2 = (uint16_t)((dutyCycle >> 19) & 0x7FF); // ARR = 2047 => 0x7FF(2047+1) ticks per period According to CubeMX settings for TIM3)
																												// ARR = 11 bits => CCR can only be 11 bits => shift dutyCycle 19 bits	
	} 
	else if(dutyCycle < 0) // Counter-clockwise: use CCR1, keep CCR2 low.
	{
			TIM3->CCR1 = (uint16_t)((-dutyCycle >> 19) & 0x7FF); // ARR = 2047 => 0x7FF(2047+1) ticks per period According to CubeMX settings for TIM3)
			
			TIM3->CCR2 = 0;
	} 
	else // Motor off
	{
			TIM3->CCR1 = 0;
			TIM3->CCR2 = 0;
	}
}

/* ----------------- Encoder velocity ----------------- */

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
	
	if (milliSecondsPrevious == 0U) 
	{
			counterPreviousTIM1 = counter;
			milliSecondsPrevious = ms;

			return 0;
	}
	
	uint16_t counterDifference      = counter - counterPreviousTIM1;
	uint32_t milliSecondsDifference = ms - milliSecondsPrevious;
	
	if(counterDifference == 0 || milliSecondsDifference == 0)
		return 0;
	
	velocityRPM = (int32_t)(((int64_t)counterDifference * 60000) / (ENCODER_COUNTS_PER_REV * milliSecondsDifference));
	
	if(counterPreviousTIM1 > counter) // Handles the reverse case
		velocityRPM = -1 * velocityRPM;
	
	counterPreviousTIM1      = counter;
	milliSecondsPrevious     = ms;
	
	return velocityRPM;
}