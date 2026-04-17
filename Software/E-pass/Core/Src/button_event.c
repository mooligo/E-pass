#include "button_event.h"

#include "main.h"

static volatile uint8_t sPrevPending = 0U;
static volatile uint8_t sNextPending = 0U;

void ButtonEvent_Init(void)
{
    sPrevPending = 0U;
    sNextPending = 0U;
}

uint8_t ButtonEvent_HasPending(void)
{
    return (uint8_t)((sPrevPending != 0U) || (sNextPending != 0U));
}

ButtonEvent_t ButtonEvent_Get(void)
{
    if (sPrevPending != 0U)
    {
        sPrevPending = 0U;
        return BUTTON_EVENT_PREV;
    }

    if (sNextPending != 0U)
    {
        sNextPending = 0U;
        return BUTTON_EVENT_NEXT;
    }

    return BUTTON_EVENT_NONE;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == Button_1_Pin)
    {
        sPrevPending = 1U;
    }
    else if (GPIO_Pin == Button_2_Pin)
    {
        sNextPending = 1U;
    }
}
