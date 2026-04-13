/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    display_spi.c
  * @brief   ILI9341-style SPI display interface abstraction.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "display_spi.h"

/* USER CODE BEGIN 0 */

/* ILI9341 standard commands used by this transport helper. */
#define DISPLAY_CMD_CASET 0x2A
#define DISPLAY_CMD_PASET 0x2B
#define DISPLAY_CMD_RAMWR 0x2C
#define DISPLAY_CMD_SWRESET 0x01
#define DISPLAY_CMD_SLPOUT 0x11
#define DISPLAY_CMD_MADCTL 0x36
#define DISPLAY_CMD_COLMOD 0x3A
#define DISPLAY_CMD_NORON 0x13
#define DISPLAY_CMD_DISPON 0x29

#define DISPLAY_SPI_TIMEOUT_MS 100U
#define DISPLAY_WIDTH 240U
#define DISPLAY_HEIGHT 320U
#define DISPLAY_FILL_CHUNK_PIXELS 240U

static DisplaySPI_Status_t DisplaySPI_Write(DisplaySPI_Handle_t *display,
                                            GPIO_PinState dcState,
                                            const uint8_t *data,
                                            size_t len)
{
  if ((display == NULL) || (display->cfg.hspi == NULL))
  {
    return DISPLAY_SPI_ERR_PARAM;
  }

  if (!display->isReady)
  {
    return DISPLAY_SPI_ERR_NOT_READY;
  }

  if ((data == NULL) || (len == 0U))
  {
    return DISPLAY_SPI_ERR_PARAM;
  }

  if (display->cfg.csPort != NULL)
  {
    HAL_GPIO_WritePin(display->cfg.csPort, display->cfg.csPin, GPIO_PIN_RESET);
  }

  if (display->cfg.dcPort != NULL)
  {
    HAL_GPIO_WritePin(display->cfg.dcPort, display->cfg.dcPin, dcState);
  }

  if (HAL_SPI_Transmit(display->cfg.hspi, (uint8_t *)data, (uint16_t)len, DISPLAY_SPI_TIMEOUT_MS) != HAL_OK)
  {
    if (display->cfg.csPort != NULL)
    {
      HAL_GPIO_WritePin(display->cfg.csPort, display->cfg.csPin, GPIO_PIN_SET);
    }
    return DISPLAY_SPI_ERR_HAL;
  }

  if (display->cfg.csPort != NULL)
  {
    HAL_GPIO_WritePin(display->cfg.csPort, display->cfg.csPin, GPIO_PIN_SET);
  }

  return DISPLAY_SPI_OK;
}

/* USER CODE END 0 */

DisplaySPI_Status_t DisplaySPI_Init(DisplaySPI_Handle_t *display, const DisplaySPI_Config_t *cfg)
{
  if ((display == NULL) || (cfg == NULL) || (cfg->hspi == NULL))
  {
    return DISPLAY_SPI_ERR_PARAM;
  }

  display->cfg = *cfg;
  display->isReady = 1U;

  if (display->cfg.csPort != NULL)
  {
    HAL_GPIO_WritePin(display->cfg.csPort, display->cfg.csPin, GPIO_PIN_SET);
  }

  if (display->cfg.dcPort != NULL)
  {
    HAL_GPIO_WritePin(display->cfg.dcPort, display->cfg.dcPin, GPIO_PIN_SET);
  }

  if (display->cfg.rstPort != NULL)
  {
    (void)DisplaySPI_Reset(display);
  }

  if (display->cfg.blPort != NULL)
  {
    (void)DisplaySPI_SetBacklight(display, 1U);
  }

  return DISPLAY_SPI_OK;
}

DisplaySPI_Status_t DisplaySPI_Reset(DisplaySPI_Handle_t *display)
{
  if ((display == NULL) || (!display->isReady))
  {
    return DISPLAY_SPI_ERR_PARAM;
  }

  if (display->cfg.rstPort == NULL)
  {
    return DISPLAY_SPI_OK;
  }

  HAL_GPIO_WritePin(display->cfg.rstPort, display->cfg.rstPin, GPIO_PIN_RESET);
  HAL_Delay(10U);
  HAL_GPIO_WritePin(display->cfg.rstPort, display->cfg.rstPin, GPIO_PIN_SET);
  HAL_Delay(120U);

  return DISPLAY_SPI_OK;
}

DisplaySPI_Status_t DisplaySPI_WriteCommand(DisplaySPI_Handle_t *display, uint8_t cmd)
{
  return DisplaySPI_Write(display, GPIO_PIN_RESET, &cmd, 1U);
}

DisplaySPI_Status_t DisplaySPI_WriteData(DisplaySPI_Handle_t *display, const uint8_t *data, size_t len)
{
  return DisplaySPI_Write(display, GPIO_PIN_SET, data, len);
}

DisplaySPI_Status_t DisplaySPI_WriteData16(DisplaySPI_Handle_t *display, const uint16_t *data, size_t len)
{
  return DisplaySPI_WriteData(display, (const uint8_t *)data, len * sizeof(uint16_t));
}

DisplaySPI_Status_t DisplaySPI_SetAddressWindow(DisplaySPI_Handle_t *display,
                                                uint16_t x0,
                                                uint16_t y0,
                                                uint16_t x1,
                                                uint16_t y1)
{
  uint8_t frame[4];

  if ((x1 < x0) || (y1 < y0))
  {
    return DISPLAY_SPI_ERR_PARAM;
  }

  if (DisplaySPI_WriteCommand(display, DISPLAY_CMD_CASET) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }

  frame[0] = (uint8_t)(x0 >> 8);
  frame[1] = (uint8_t)(x0 & 0xFFU);
  frame[2] = (uint8_t)(x1 >> 8);
  frame[3] = (uint8_t)(x1 & 0xFFU);
  if (DisplaySPI_WriteData(display, frame, sizeof(frame)) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }

  if (DisplaySPI_WriteCommand(display, DISPLAY_CMD_PASET) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }

  frame[0] = (uint8_t)(y0 >> 8);
  frame[1] = (uint8_t)(y0 & 0xFFU);
  frame[2] = (uint8_t)(y1 >> 8);
  frame[3] = (uint8_t)(y1 & 0xFFU);
  if (DisplaySPI_WriteData(display, frame, sizeof(frame)) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }

  return DISPLAY_SPI_OK;
}

DisplaySPI_Status_t DisplaySPI_BeginMemoryWrite(DisplaySPI_Handle_t *display)
{
  return DisplaySPI_WriteCommand(display, DISPLAY_CMD_RAMWR);
}

DisplaySPI_Status_t DisplaySPI_WritePixelsRGB565(DisplaySPI_Handle_t *display, const uint16_t *pixels, size_t count)
{
  if ((pixels == NULL) || (count == 0U))
  {
    return DISPLAY_SPI_ERR_PARAM;
  }

  return DisplaySPI_WriteData16(display, pixels, count);
}

DisplaySPI_Status_t DisplaySPI_SetBacklight(DisplaySPI_Handle_t *display, uint8_t on)
{
  if ((display == NULL) || (!display->isReady))
  {
    return DISPLAY_SPI_ERR_PARAM;
  }

  if (display->cfg.blPort == NULL)
  {
    return DISPLAY_SPI_OK;
  }

  if (on)
  {
    HAL_GPIO_WritePin(display->cfg.blPort, display->cfg.blPin, display->cfg.activeBacklightState);
  }
  else
  {
    HAL_GPIO_WritePin(display->cfg.blPort,
                      display->cfg.blPin,
                      (display->cfg.activeBacklightState == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET);
  }

  return DISPLAY_SPI_OK;
}

DisplaySPI_Status_t DisplaySPI_InitPanel(DisplaySPI_Handle_t *display)
{
  uint8_t param;

  if ((display == NULL) || (!display->isReady))
  {
    return DISPLAY_SPI_ERR_PARAM;
  }

  if (DisplaySPI_Reset(display) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }

  if (DisplaySPI_WriteCommand(display, DISPLAY_CMD_SWRESET) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }
  HAL_Delay(120U);

  if (DisplaySPI_WriteCommand(display, DISPLAY_CMD_SLPOUT) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }
  HAL_Delay(120U);

  param = 0x55U; /* 16-bit/pixel RGB565 */
  if (DisplaySPI_WriteCommand(display, DISPLAY_CMD_COLMOD) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }
  if (DisplaySPI_WriteData(display, &param, 1U) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }

  param = 0x48U; /* MX + BGR, portrait default */
  if (DisplaySPI_WriteCommand(display, DISPLAY_CMD_MADCTL) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }
  if (DisplaySPI_WriteData(display, &param, 1U) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }

  if (DisplaySPI_WriteCommand(display, DISPLAY_CMD_NORON) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }
  HAL_Delay(10U);

  if (DisplaySPI_WriteCommand(display, DISPLAY_CMD_DISPON) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }
  HAL_Delay(20U);

  return DISPLAY_SPI_OK;
}

DisplaySPI_Status_t DisplaySPI_FillScreenRGB565(DisplaySPI_Handle_t *display, uint16_t color)
{
  static uint16_t fillChunk[DISPLAY_FILL_CHUNK_PIXELS];
  size_t pixelsRemaining = (size_t)DISPLAY_WIDTH * (size_t)DISPLAY_HEIGHT;
  size_t i;

  if ((display == NULL) || (!display->isReady))
  {
    return DISPLAY_SPI_ERR_PARAM;
  }

  for (i = 0U; i < DISPLAY_FILL_CHUNK_PIXELS; i++)
  {
    fillChunk[i] = color;
  }

  if (DisplaySPI_SetAddressWindow(display, 0U, 0U, DISPLAY_WIDTH - 1U, DISPLAY_HEIGHT - 1U) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }

  if (DisplaySPI_BeginMemoryWrite(display) != DISPLAY_SPI_OK)
  {
    return DISPLAY_SPI_ERR_HAL;
  }

  while (pixelsRemaining > 0U)
  {
    size_t chunk = (pixelsRemaining > DISPLAY_FILL_CHUNK_PIXELS) ? DISPLAY_FILL_CHUNK_PIXELS : pixelsRemaining;
    if (DisplaySPI_WritePixelsRGB565(display, fillChunk, chunk) != DISPLAY_SPI_OK)
    {
      return DISPLAY_SPI_ERR_HAL;
    }
    pixelsRemaining -= chunk;
  }

  return DISPLAY_SPI_OK;
}
