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

static void Timer2_Init_10ms(void)
{
    // Enable TIM2 clock
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

    // Reset timer
    TIM2->CR1 = 0;
    TIM2->PSC = 7999;   // Prescaler
    TIM2->ARR = 49;      // Auto-reload ? 10 ms
    TIM2->EGR = TIM_EGR_UG;

    // Enable update interrupt
    TIM2->DIER |= TIM_DIER_UIE;

    // Enable timer
    TIM2->CR1 |= TIM_CR1_CEN;

    // Enable IRQ in NVIC
    NVIC_SetPriority(TIM2_IRQn, 5);
    NVIC_EnableIRQ(TIM2_IRQn);
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
	
		// Initialize hardware timer BEFORE kernel start
    Timer2_Init_10ms();
	
		osKernelStart();        //  scheduler takes over
}



/* Threads ------------------------------------------------------------------*/
static void app_ctrl(void *argument)
{
		(void)argument;

    for (;;)
    {
        
				osThreadFlagsWait(CTRL_FLAG, osFlagsWaitAny, osWaitForever);
				millisec = Main_GetTickMillisec();
        velocity = Peripheral_Encoder_CalculateVelocity(millisec);
        control  = Controller_PIController(&reference, &velocity, &millisec);
        Peripheral_PWM_ActuateMotor(control);
        //osDelay(10);
    }
}

static void app_ref(void *argument)
{
    (void)argument;

    for (;;)
    {
			osThreadFlagsWait(REF_FLAG, osFlagsWaitAny, osWaitForever);	
			reference = -reference;
      //osDelay(4000);
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

void TIM2_IRQHandler(void)
{
    static uint32_t tick = 0;

    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;   // Clear update interrupt flag
        tick++;

        osThreadFlagsSet(ctrl_tid, CTRL_FLAG);

        if (tick % 400 == 0) {     // 400 × 10 ms = 4 s
            osThreadFlagsSet(ref_tid, REF_FLAG);
        }
    }
}

