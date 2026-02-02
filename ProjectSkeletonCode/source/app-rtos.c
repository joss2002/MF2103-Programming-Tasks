#include "main.h"
#include "cmsis_os2.h"
#include "application.h"
#include "controller.h"
#include "peripherals.h"

#define CTRL_FLAG  (1U << 0)
#define REF_FLAG   (1U << 1)

//  STATUS UPDATE: TASK 2 delayos works, timer based event trigger encounters size restriction

/* Global variables ----------------------------------------------------------*/
int32_t reference, velocity, control;
uint32_t millisec;


/* Thread prototypes ---------------------------------------------------------*/
static void app_ctrl(void *argument);
static void app_ref(void *argument);
static void app_main(void *argument);

// Timer callback prototype
static void scheduler_timer_cb(void *argument);

//IDs for the control and the reference threads, used for setting flags
static osThreadId_t ctrl_tid;
static osThreadId_t ref_tid;
static osTimerId_t scheduler_timer;


/* Thread attributes ---------------------------------------------------------*/
static const osThreadAttr_t app_ctrl_attr = {
    .name = "app_ctrl",
    .priority = osPriorityHigh   // control loop must preempt reference
};

static const osThreadAttr_t app_ref_attr = {
    .name = "app_ref",
    .priority = osPriorityLow
};

static const osThreadAttr_t app_main_attr = {
    .name = "app_main",
    .priority = osPriorityBelowNormal
};

//Timer attributes
static void scheduler_timer_cb(void *argument)
{
    static uint32_t tick = 0;
    tick++;

    // Control loop every 10 ms
    osThreadFlagsSet(ctrl_tid, CTRL_FLAG);

    // Reference flip every 4000 ms
    if (tick % 400 == 0) {
        osThreadFlagsSet(ref_tid, REF_FLAG);
    }
}


/* Setup --------------------------------------------------------------------*/
void Application_Setup(void)
{
    reference = 2000;
    velocity  = 0;
    control   = 0;
    millisec  = 0;

    Peripheral_GPIO_EnableMotor();
    Controller_Reset();
		osKernelInitialize();
	
		ctrl_tid = osThreadNew(app_ctrl, NULL, &app_ctrl_attr);
		ref_tid  = osThreadNew(app_ref,  NULL, &app_ref_attr);
		osThreadNew(app_main, NULL, &app_main_attr);
	
	/* The following section is for timer flag*/
	//		scheduler_timer = osTimerNew(
	//        scheduler_timer_cb,
	//        osTimerPeriodic,
	//        NULL,
	//        NULL
	//    );

    osTimerStart(scheduler_timer, 10); // 10 ms base tick
		osKernelStart();        //  scheduler takes over
}



/* Threads ------------------------------------------------------------------*/
static void app_ctrl(void *argument)
{
		(void)argument;

    for (;;)
    {
        
				//osThreadFlagsWait(CTRL_FLAG, osFlagsWaitAny, osWaitForever);
				millisec = Main_GetTickMillisec();
        velocity = Peripheral_Encoder_CalculateVelocity(millisec);
        control  = Controller_PIController(&reference, &velocity, &millisec);
        Peripheral_PWM_ActuateMotor(control);
        osDelay(10);
    }
}

static void app_ref(void *argument)
{
    (void)argument;

    for (;;)
    {
			//osThreadFlagsWait(REF_FLAG, osFlagsWaitAny, osWaitForever);	
			reference = -reference;
      osDelay(4000);
    }
}

static void app_main(void *argument)
{
    (void)argument;

    for (;;)
    {
        Application_Loop();
    }
}

/* Application loop (PASSIVE BY DESIGN) -------------------------------------*/
void Application_Loop(void)
{
    osThreadFlagsWait(0x01, osFlagsWaitAll, osWaitForever);
}
