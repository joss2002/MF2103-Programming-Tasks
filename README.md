Instructions 

1) Make sure to have included everything mentioned in task 2 pre requsites
2)Copy paste the following for application.h

#ifndef _Application_H_
#define _Application_H_
#ifdef __cplusplus
extern "C" {
#endif

#define PERIOD_CTRL 10		//!< Period of the control loop in milliseconds.
#define PERIOD_REF 4000		//!< Period of the reference switch in milliseconds.

/**
 * @brief Initializes the application.
 *
 * This function is responsible for initializing the application.
 * It should set up peripherals, variables, libraries, and any other initial 
 * configurations necessary for the application.
 * It doesn't take any arguments and doesn't return any value.
 */
void Application_Setup(void);

/**
 * @brief Main application loop.
 *
 * This function contains the main loop that runs continuously after initialization.
 * It handles the core functionality of the application.
 * It doesn't take any arguments and doesn't return any value.
 */
void Application_Loop(void);

/* RTOS entry point (defined in app-rtos.c) */
void MX_RTOS_Init(void);

#ifdef __cplusplus
}
#endif

#endif   // _Application_H_

3) When running app-rtos.c, make sure the following code

void SysTick_Handler(void)
{
  HAL_IncTick();
}

in Project-->STM32CubeMX:Common Sources -> stm32l4xx_it.c is commented out


4) Make the following RTE modifications
// Enable CMSIS -> ostick->sys tick 
// Enable CMSIS -> RTOS2 API -> rtx5
// CMSIS-View-> Event recorder

