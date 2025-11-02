#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_rtc.h"
#include "system_stm32f10x.h"
#include <stdio.h>

// Hàm
void UART1_Config(void);
void RTC_Config(void);
void SendString(const char *str);
void DelayMs(uint32_t ms);
void Safe_Enter_Standby(void);

int main(void)
{
    SystemInit();
    UART1_Config();

    // === CH? HI?N TH? WAKEUP HO?C NORMAL BOOT ===
    if (PWR_GetFlagStatus(PWR_FLAG_SB) != RESET)
    {
        PWR_ClearFlag(PWR_FLAG_SB);
        SendString("\r\n>>> WAKEUP <<<\r\n");
    }
    else
    {
        SendString("\r\n>>> BOOT <<<\r\n");
    }

    RTC_Config();
    RTC_ITConfig(RTC_IT_ALR, ENABLE);
    RTC_WaitForLastTask();

    // === 5 GIÂY: G?I CHU?I Ð?M ===
    for (int i = 1; i <= 5; i++)
    {
        char buffer[32];
        sprintf(buffer, "STM32 is running... %d\r\n", i);
        SendString(buffer);
        DelayMs(1000);
    }

    // === T? Ð?NG VÀO STANDBY (KHÔNG THÔNG BÁO) ===
    RTC_SetAlarm(RTC_GetCounter() + 10);
    RTC_WaitForLastTask();
    Safe_Enter_Standby();

    while (1); // Không bao gi? t?i
}

/* ========================================
   UART1 CONFIG (PA9=TX, PA10=RX)
   ======================================== */
void UART1_Config(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);

    // PA9 = TX
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // PA10 = RX
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate            = 115200;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);

    USART_Cmd(USART1, ENABLE);
}

/* ========================================
   G?I CHU?I AN TOÀN
   ======================================== */
void SendString(const char *str)
{
    while (*str)
    {
        USART_SendData(USART1, *str++);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}

/* ========================================
   RTC CONFIG (LSE)
   ======================================== */
void RTC_Config(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    RCC_LSEConfig(RCC_LSE_ON);
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);

    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE);
    RTC_WaitForSynchro();

    RTC_SetPrescaler(32767);
    RTC_WaitForLastTask();
}

/* ========================================
   DELAY CHÍNH XÁC
   ======================================== */
void DelayMs(uint32_t ms)
{
    volatile uint32_t i;
    for (i = 0; i < ms * 7200; i++)
        __NOP();
}

/* ========================================
   VÀO STANDBY
   ======================================== */
void Safe_Enter_Standby(void)
{
    PWR_WakeUpPinCmd(ENABLE);
    PWR_ClearFlag(PWR_FLAG_WU | PWR_FLAG_SB);
    PWR_EnterSTANDBYMode();
}