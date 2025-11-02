#include "stm32f10x.h"

// Ch?n 1 trong 2 ch? d?:
//#define USE_DELAY_MODE
#define USE_SLEEP_MODE

volatile uint8_t tick_flag = 0;

void LED_Init(void);
void delay_ms(uint32_t ms);
void Clock_To_72MHz(void);
void SysTick_Handler(void);

int main(void)
{
    Clock_To_72MHz();
    LED_Init();

    // Dù ? mode nào cung b?t SysTick d? tránh sleep mãi
    SysTick_Config(SystemCoreClock / 1000); // tick m?i 1ms

#ifdef USE_DELAY_MODE
    while (1)
    {
        GPIO_SetBits(GPIOB, GPIO_Pin_12);
        delay_ms(1000);
        GPIO_ResetBits(GPIOB, GPIO_Pin_12);
        delay_ms(1000);
    }

#elif defined(USE_SLEEP_MODE)
    while (1)
    {
        if (tick_flag)
        {
            tick_flag = 0;
            GPIOB->ODR ^= GPIO_Pin_12;  // d?o LED m?i giây
        }

        __WFI();  // CPU ng? t?m th?i, v?n th?c d?y nh? SysTick
    }
#endif
}

/* =================== LED INIT =================== */
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* =================== DELAY =================== */
void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms * 8000; i++)
        __NOP(); // CPU b?n, tiêu th? cao
}

/* =================== SysTick Handler =================== */
void SysTick_Handler(void)
{
    static uint16_t cnt = 0;
    cnt++;
    if (cnt >= 1000)
    {
        cnt = 0;
        tick_flag = 1;
    }
}

/* =================== Clock 72 MHz =================== */
void Clock_To_72MHz(void)
{
    RCC_DeInit();
    RCC_HSEConfig(RCC_HSE_ON);
    while (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET);

    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
    RCC_PLLCmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while (RCC_GetSYSCLKSource() != 0x08);
    SystemCoreClockUpdate();
}
