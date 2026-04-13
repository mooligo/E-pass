/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    button_logic.c
  * @brief   Debounced multi-button logic and event generation.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "button_logic.h"

/* USER CODE BEGIN 0 */

static uint8_t ButtonLogic_ReadRawPressed(const ButtonState_t *state)
{
  GPIO_PinState pinState;

  if ((state == NULL) || (state->cfg.port == NULL))
  {
    return 0U;
  }

  pinState = HAL_GPIO_ReadPin(state->cfg.port, state->cfg.pin);
  return (pinState == state->cfg.activeState) ? 1U : 0U;
}

static void ButtonLogic_PushEvent(ButtonLogic_Handle_t *handle, const ButtonState_t *state, ButtonEventType_t type)
{
  ButtonEvent_t *slot;

  if ((handle == NULL) || (state == NULL) || (handle->queueCount >= BUTTON_LOGIC_EVENT_QUEUE_SIZE))
  {
    return;
  }

  slot = &handle->queue[handle->queueTail];
  slot->buttonId = state->cfg.id;
  slot->type = type;
  slot->timestampMs = HAL_GetTick();

  handle->queueTail = (uint8_t)((handle->queueTail + 1U) % BUTTON_LOGIC_EVENT_QUEUE_SIZE);
  handle->queueCount++;
}

/* USER CODE END 0 */

HAL_StatusTypeDef ButtonLogic_Init(ButtonLogic_Handle_t *handle,
                                   const ButtonConfig_t *configs,
                                   uint8_t count)
{
  uint8_t i;

  if ((handle == NULL) || (configs == NULL) || (count == 0U) || (count > BUTTON_LOGIC_MAX_BUTTONS))
  {
    return HAL_ERROR;
  }

  handle->count = count;
  handle->queueHead = 0U;
  handle->queueTail = 0U;
  handle->queueCount = 0U;

  for (i = 0U; i < count; i++)
  {
    handle->states[i].cfg = configs[i];
    handle->states[i].stablePressed = 0U;
    handle->states[i].rawPressed = 0U;
    handle->states[i].longPressSent = 0U;
    handle->states[i].lastRawChangeTick = HAL_GetTick();
    handle->states[i].pressStartTick = 0U;
  }

  return HAL_OK;
}

void ButtonLogic_Process(ButtonLogic_Handle_t *handle, uint32_t nowMs)
{
  uint8_t i;

  if (handle == NULL)
  {
    return;
  }

  for (i = 0U; i < handle->count; i++)
  {
    ButtonState_t *state = &handle->states[i];
    uint8_t raw = ButtonLogic_ReadRawPressed(state);

    if (raw != state->rawPressed)
    {
      state->rawPressed = raw;
      state->lastRawChangeTick = nowMs;
    }

    if ((nowMs - state->lastRawChangeTick) >= state->cfg.debounceMs)
    {
      if (state->stablePressed != state->rawPressed)
      {
        state->stablePressed = state->rawPressed;

        if (state->stablePressed)
        {
          state->pressStartTick = nowMs;
          state->longPressSent = 0U;
          ButtonLogic_PushEvent(handle, state, BUTTON_EVENT_PRESS);
        }
        else
        {
          ButtonLogic_PushEvent(handle, state, BUTTON_EVENT_RELEASE);

          if (!state->longPressSent)
          {
            ButtonLogic_PushEvent(handle, state, BUTTON_EVENT_CLICK);
          }
        }
      }
    }

    if (state->stablePressed && !state->longPressSent)
    {
      if ((nowMs - state->pressStartTick) >= state->cfg.longPressMs)
      {
        state->longPressSent = 1U;
        ButtonLogic_PushEvent(handle, state, BUTTON_EVENT_LONG_PRESS);
      }
    }
  }
}

uint8_t ButtonLogic_GetEvent(ButtonLogic_Handle_t *handle, ButtonEvent_t *eventOut)
{
  if ((handle == NULL) || (eventOut == NULL))
  {
    return 0U;
  }

  if (handle->queueCount == 0U)
  {
    return 0U;
  }

  *eventOut = handle->queue[handle->queueHead];
  handle->queueHead = (uint8_t)((handle->queueHead + 1U) % BUTTON_LOGIC_EVENT_QUEUE_SIZE);
  handle->queueCount--;

  return 1U;
}

uint8_t ButtonLogic_IsPressed(const ButtonLogic_Handle_t *handle, uint8_t buttonId)
{
  uint8_t i;

  if (handle == NULL)
  {
    return 0U;
  }

  for (i = 0U; i < handle->count; i++)
  {
    if (handle->states[i].cfg.id == buttonId)
    {
      return handle->states[i].stablePressed;
    }
  }

  return 0U;
}
