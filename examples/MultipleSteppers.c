#include <stdio.h>
#include "air105.h"
#include "sysc.h"
#include "delay.h"
#include "usart.h"
#include "air105_gpio.h"

#include "TeensyStepC.h"

Stepper motor1 = { 0 };
Stepper motor2 = { 0 };
Stepper motor3 = { 0 };
Stepper motor4 = { 0 };
Stepper motor5 = { 0 };

StepControl controller = { 0 };

Stepper* steppers[MAXMOTORS] = {&motor1, &motor2, &motor3, &motor4, &motor5};

static void callback(int32_t target){
    printf("target: %ld\r\n", target);
}

int main(void)
{
    SystemClock_Config_HSE();
    Delay_Init();
    USART_Init();

    printf("USART Init\r\n");

    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15 | GPIO_Pin_12 | GPIO_Pin_13;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Remap = GPIO_Remap_1;
    GPIO_Init(GPIOD, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Remap = GPIO_Remap_1;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_5;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Remap = GPIO_Remap_1;
    GPIO_Init(GPIOB, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Remap = GPIO_Remap_1;
    GPIO_Init(GPIOC, &gpio);

    printf("GPIO Init\r\n");
	
    Delay_ms(1000);

    SYSCTRL_APBPeriphClockCmd(SYSCTRL_APBPeriph_TIMM0, ENABLE);

    uint32_t timer_clk = SYSCTRL->PCLK_1MS_VAL * 1000;
    TIM_Init(TIMM0, &(TIM_InitTypeDef){TIM_1, timer_clk / 100000});
    NVIC_SetPriority(TIM0_1_IRQn, 3);
    TIM_ITConfig(TIMM0, TIM_1, ENABLE);
    NVIC_EnableIRQ(TIM0_1_IRQn);
	// TIM_Cmd(TIMM0, TIM_1, ENABLE);
    
    TIM_Init(TIMM0, &(TIM_InitTypeDef){TIM_2, timer_clk / 100000});
    NVIC_SetPriority(TIM0_2_IRQn, 4);
    TIM_ITConfig(TIMM0, TIM_2, ENABLE);
    NVIC_EnableIRQ(TIM0_2_IRQn);
    // TIM_Cmd(TIMM0, TIM_2, ENABLE);

    TIM_Init(TIMM0, &(TIM_InitTypeDef){TIM_3, timer_clk / 100000});
    NVIC_SetPriority(TIM0_3_IRQn, 2);
    TIM_ITConfig(TIMM0, TIM_3, ENABLE);
    NVIC_EnableIRQ(TIM0_3_IRQn);
    // TIM_Cmd(TIMM0, TIM_3, ENABLE);
		
		

    TimerField_init(&controller.controller.timerField, 
              &(TimerField_InitTypeDef){.stepTimer = TIM_1, .accTimer = TIM_2, .pulseTimer = TIM_3});

    Stepper_init(&motor1, 
                &(Stepper_InitTypeDef){ .dirPin = {.pin = GPIO_Pin_14, .port = (uint32_t)GPIOD}, 
								        .stepPin = {.pin = GPIO_Pin_15, .port = (uint32_t)GPIOD} });
    Stepper_init(&motor2, 
                &(Stepper_InitTypeDef){ .dirPin = {.pin = GPIO_Pin_12, .port = (uint32_t)GPIOD}, 
								        .stepPin = {.pin = GPIO_Pin_13, .port = (uint32_t)GPIOD} });
    Stepper_init(&motor3, 
                &(Stepper_InitTypeDef){ .dirPin = {.pin = GPIO_Pin_6, .port = (uint32_t)GPIOA}, 
								        .stepPin = {.pin = GPIO_Pin_7, .port = (uint32_t)GPIOA} });
    Stepper_init(&motor4, 
                &(Stepper_InitTypeDef){ .dirPin = {.pin = GPIO_Pin_2, .port = (uint32_t)GPIOB}, 
								        .stepPin = {.pin = GPIO_Pin_5, .port = (uint32_t)GPIOB} });
    Stepper_init(&motor5, 
                &(Stepper_InitTypeDef){ .dirPin = {.pin = GPIO_Pin_12, .port = (uint32_t)GPIOC}, 
								        .stepPin = {.pin = GPIO_Pin_13, .port = (uint32_t)GPIOC} });


    StepControl_init(&controller, 
                     &(StepControl_Init_TypeDef){.accUpdatePeriod = 1000, 
										                             .pulseWidth = 5, 
										                             .reachedTargetCallback = callback});

    uint32_t sqr = 50000;
    for(int i = 0; i < 5; i++){
        Stepper_setMaxSpeed(steppers[i], 50000);
	    Stepper_setAcceleration(steppers[i], 200000);
        Stepper_setPullInOutSpeed(steppers[i], 10000, 10000);
        Stepper_setTargetRel(steppers[i], sqr / 4);
	    Stepper_setStepPinPolarity(steppers[i], HIGH);
    }

    StepControl_move(&controller, 1.0f, 5, steppers);
    while (1)
    {
        // GPIO_ResetBits(GPIOD, GPIO_Pin_14|GPIO_Pin_15);
//        GPIOD->BSRR = ((uint32_t)GPIO_Pin_15) << 16;
//        printf("RESET ");
//        Delay_ms(1000);
        // GPIO_SetBits(GPIOD, GPIO_Pin_14|GPIO_Pin_15);
//        GPIOD->BSRR = (GPIO_Pin_15);
//        printf("SET\r\n");
        Delay_ms(100);
        
        for(int i = 0; i < 5; i++){
            Stepper_setTargetRel(steppers[i], sqr);
        }
        StepControl_move(&controller, 1.0f, 5, steppers);
    }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

     /* Infinite loop */
    while (1)
    {
    }
}
#endif
