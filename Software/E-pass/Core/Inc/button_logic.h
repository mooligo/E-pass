/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    button_logic.h
  * @brief   Debounced multi-button logic and event generation.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef BUTTON_LOGIC_H
#define BUTTON_LOGIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

#define BUTTON_LOGIC_MAX_BUTTONS 4U
#define BUTTON_LOGIC_EVENT_QUEUE_SIZE 16U

typedef enum
{
  BUTTON_EVENT_NONE = 0,
  BUTTON_EVENT_PRESS,
  BUTTON_EVENT_RELEASE,
  BUTTON_EVENT_CLICK,
  BUTTON_EVENT_LONG_PRESS
} ButtonEventType_t;

typedef struct
{
  uint8_t buttonId;
  ButtonEventType_t type;
  uint32_t timestampMs;
} ButtonEvent_t;

typedef struct
{
  uint8_t id;
  GPIO_TypeDef *port;
  uint16_t pin;
  GPIO_PinState activeState;
  uint32_t debounceMs;
  uint32_t longPressMs;
} ButtonConfig_t;

typedef struct
{
  ButtonConfig_t cfg;
  uint8_t stablePressed;
  uint8_t rawPressed;
  uint8_t longPressSent;
  uint32_t lastRawChangeTick;
  uint32_t pressStartTick;
} ButtonState_t;

typedef struct
{
  ButtonState_t states[BUTTON_LOGIC_MAX_BUTTONS];
  uint8_t count;
  ButtonEvent_t queue[BUTTON_LOGIC_EVENT_QUEUE_SIZE];
  uint8_t queueHead;
  uint8_t queueTail;
  uint8_t queueCount;
} ButtonLogic_Handle_t;

/* USER CODE BEGIN Prototypes */

HAL_StatusTypeDef ButtonLogic_Init(ButtonLogic_Handle_t *handle,
                                   const ButtonConfig_t *configs,
                                   uint8_t count);
void ButtonLogic_Process(ButtonLogic_Handle_t *handle, uint32_t nowMs);
uint8_t ButtonLogic_GetEvent(ButtonLogic_Handle_t *handle, ButtonEvent_t *eventOut);
uint8_t ButtonLogic_IsPressed(const ButtonLogic_Handle_t *handle, uint8_t buttonId);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_LOGIC_H */
