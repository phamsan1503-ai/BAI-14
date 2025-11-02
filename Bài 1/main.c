#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

/* ===== Event Bits ===== */
#define TASK_A_BIT   (1 << 0)
#define TASK_B_BIT   (1 << 1)
#define TASK_C_BIT   (1 << 2)

/* ===== Function Prototypes ===== */
static void System_Config(void);
static void GPIO_Config(void);
static void USART_Config(void);
static void UART_SendChar(char ch);
static void UART_SendStr(const char *str);

static void vTaskMain(void *pvParameters);
static void vTaskHandler(void *pvParameters);

/* ===== Global handle ===== */
EventGroupHandle_t xEventGroup;

/* ===== C?u trúc thông tin task ===== */
typedef struct {
    uint32_t bitMask;
    uint16_t ledPin;
    char name[8];
} TaskInfo_t;

/* ===== Danh sách task ===== */
TaskInfo_t taskList[] = {
    { TASK_A_BIT, GPIO_Pin_12, "Task A" },
    { TASK_B_BIT, GPIO_Pin_13, "Task B" },
    { TASK_C_BIT, GPIO_Pin_14, "Task C" }
};

/* ===================== MAIN ===================== */
int main(void)
{
    System_Config();

    xEventGroup = xEventGroupCreate();

    /* Task phát tín hi?u */
    xTaskCreate(vTaskMain, "Main", 128, NULL, 3, NULL);

    /* T?o các task x? lý */
    for (int i = 0; i < 3; i++) {
        xTaskCreate(vTaskHandler, taskList[i].name, 128, &taskList[i], 2, NULL);
    }

    vTaskStartScheduler();

    while (1);
}

/* ===================== Task Main ===================== */
static void vTaskMain(void *pvParameters)
{
    (void)pvParameters;

    while (1)
    {
        /* B?t l?n lu?t t?ng task */
        xEventGroupSetBits(xEventGroup, TASK_A_BIT);
        vTaskDelay(pdMS_TO_TICKS(1000));

        xEventGroupSetBits(xEventGroup, TASK_B_BIT);
        vTaskDelay(pdMS_TO_TICKS(1000));

        xEventGroupSetBits(xEventGroup, TASK_C_BIT);
        vTaskDelay(pdMS_TO_TICKS(1000));

        /* Kích ho?t t?t c? 3 task cùng lúc */
        xEventGroupSetBits(xEventGroup, TASK_A_BIT | TASK_B_BIT | TASK_C_BIT);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ===================== Task x? lý LED + UART ===================== */
static void vTaskHandler(void *pvParameters)
{
    TaskInfo_t *task = (TaskInfo_t *)pvParameters;
    EventBits_t bits;

    while (1)
    {
        bits = xEventGroupWaitBits(xEventGroup, task->bitMask, pdTRUE, pdFALSE, portMAX_DELAY);

        if (bits & task->bitMask)
        {
            /* B?t LED */
            GPIO_SetBits(GPIOB, task->ledPin);

            /* UART thông báo */
            vTaskSuspendAll();
            UART_SendStr("[");
            UART_SendStr(task->name);
            UART_SendStr("] Running\n");
            xTaskResumeAll();

            /* Gi? LED sáng 200ms r?i t?t */
            vTaskDelay(pdMS_TO_TICKS(200));
            GPIO_ResetBits(GPIOB, task->ledPin);
        }
    }
}

/* ===================== C?u hình h? th?ng ===================== */
static void System_Config(void)
{
    SystemInit();
    GPIO_Config();
    USART_Config();
}

/* ===================== GPIO Config ===================== */
static void GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14);
}

/* ===================== USART Config ===================== */
static void USART_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    // PA9 - TX
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA10 - RX
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    USART_InitStruct.USART_BaudRate = 9600;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStruct);

    USART_Cmd(USART1, ENABLE);
}

/* ===================== UART Helper ===================== */
static void UART_SendChar(char ch)
{
    USART_SendData(USART1, ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

static void UART_SendStr(const char *str)
{
    while (*str) UART_SendChar(*str++);
}
