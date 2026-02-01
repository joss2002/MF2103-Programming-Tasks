/**
 * Handles the application, of controlling motor speed, through threads.
 *
 * @authors Josefine Nyholm, 
 * 
 * @cite https://arm-software.github.io/CMSIS-RTX/latest/rtos2_tutorial.html#rtos2_tutorial_threads
 * @cite T. Martin and M. Rogers, The designer's guide to the cortex-m processor family, Second edition. 2016.
 */

#include "main.h" 

#include "application.h" 
#include "controller.h"
#include "peripherals.h"
#include "cmsis_os2.h"

/* Global variables ----------------------------------------------------------*/
int32_t reference, velocity, control;
uint32_t millisec;



osThreadId_t main_id, ctrl_id, ref_id;

/* Functions -----------------------------------------------------------------*/

static void app_main(void *arg);
void app_ctrl(void *arg);
void app_ref(void *arg);

static const osThreadAttr_t threadAttr_main = {
	.name = "app_main",
	.stack_size = 128*4,				 // Application_Loop call + waiting for flags, small call-stack => 256 bytes, with margin => 512 bytes
	.priority = osPriorityNormal
};

static const osThreadAttr_t threadAttr_ctrl = {
	.name = "app_ctrl",
	.stack_size = 128*4,         // ~24 bytes local variables, ~32 bytes RTOS-functions, ~232 bytes function calls, call-stack + safety ~100 bytes
	.priority = osPriorityNormal
};

static const osThreadAttr_t threadAttr_ref = {
	.name = "app_ref",
	.stack_size = 128*2,				 // ~8 bytes local variables, ~100-150 bytes RTOS-function, call-stack + safety ~100 bytes
	.priority = osPriorityNormal
};
 
 
/* Run setup needed for all periodic tasks */
void Application_Setup()
{
  // Reset global variables
  reference = 2000;
  velocity = 0;
  control = 0;
  millisec = 0;

  // Initialise hardware
  Peripheral_GPIO_EnableMotor();

  // Initialize controller
  Controller_Reset();
	
	osKernelInitialize();
	main_id = osThreadNew(app_main, NULL, &threadAttr_main);
	ctrl_id = osThreadNew(app_ctrl, NULL, &threadAttr_ctrl);
	ref_id  = osThreadNew(app_ref, NULL, &threadAttr_ref);
	osKernelStart();
}

/* Define what to do in the infinite loop */
void Application_Loop()
 {
   // Do nothing
   osThreadFlagsWait(0x01, osFlagsWaitAll, osWaitForever);
 }
 
/* Thread Functions -----------------------------------------------------------------*/

 /**
  * Runs additional setup and calls Application_loop() indefinetly
  */
__NO_RETURN static void app_main(void *arg)
{
	/* Additional setup code... */
	
	for(;;)
	{
		Application_Loop();
	}
}

/**
 * Samples encoder, calculates the control signal and applies it to the motor every 10 ms.
 */
__NO_RETURN void app_ctrl(void *arg)
{
	// Calculates amount of ticks representing the required period in ms
	uint32_t tickDelay = (PERIOD_CTRL * osKernelGetTickFreq()) / 1000;
	
	for(;;)
	{
		millisec = Main_GetTickMillisec();
		
		// Calculate motor velocity
		velocity = Peripheral_Encoder_CalculateVelocity(millisec);

		// Calculate control signal
		control = Controller_PIController(&reference, &velocity, &millisec);

		// Apply control signal to motor
		Peripheral_PWM_ActuateMotor(control);
		
		uint32_t runTimeTicks = Main_GetTickMillisec() - millisec;
		
		if(runTimeTicks < tickDelay)
			osDelay(tickDelay - runTimeTicks);
		else
			osDelay(1);
	}
}

/**
 * Toggles the direction of the reference every 4000 ms.
 */
__NO_RETURN void app_ref(void *arg)
{
	for(;;)
	{
		// Flip reference
		reference = -reference;
		
		// Wait for next sample
		osDelay(PERIOD_REF);
	}
}