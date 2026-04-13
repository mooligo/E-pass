/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    display_spi.h
  * @brief   ILI9341-style SPI display interface abstraction.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef DISPLAY_SPI_H
#define DISPLAY_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stddef.h>

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

typedef struct
{
  SPI_HandleTypeDef *hspi;
  GPIO_TypeDef *csPort;
  uint16_t csPin;
  GPIO_TypeDef *dcPort;
  uint16_t dcPin;
  GPIO_TypeDef *rstPort;
  uint16_t rstPin;
  GPIO_TypeDef *blPort;
  uint16_t blPin;
  GPIO_PinState activeBacklightState;
} DisplaySPI_Config_t;

typedef struct
{
  DisplaySPI_Config_t cfg;
  uint8_t isReady;
} DisplaySPI_Handle_t;

typedef enum
{
  DISPLAY_SPI_OK = 0,
  DISPLAY_SPI_ERR_PARAM,
  DISPLAY_SPI_ERR_HAL,
  DISPLAY_SPI_ERR_NOT_READY
} DisplaySPI_Status_t;

/* USER CODE BEGIN Prototypes */

DisplaySPI_Status_t DisplaySPI_Init(DisplaySPI_Handle_t *display, const DisplaySPI_Config_t *cfg);
DisplaySPI_Status_t DisplaySPI_Reset(DisplaySPI_Handle_t *display);

DisplaySPI_Status_t DisplaySPI_WriteCommand(DisplaySPI_Handle_t *display, uint8_t cmd);
DisplaySPI_Status_t DisplaySPI_WriteData(DisplaySPI_Handle_t *display, const uint8_t *data, size_t len);
DisplaySPI_Status_t DisplaySPI_WriteData16(DisplaySPI_Handle_t *display, const uint16_t *data, size_t len);

DisplaySPI_Status_t DisplaySPI_SetAddressWindow(DisplaySPI_Handle_t *display,
                                                uint16_t x0,
                                                uint16_t y0,
                                                uint16_t x1,
                                                uint16_t y1);
DisplaySPI_Status_t DisplaySPI_BeginMemoryWrite(DisplaySPI_Handle_t *display);
DisplaySPI_Status_t DisplaySPI_WritePixelsRGB565(DisplaySPI_Handle_t *display, const uint16_t *pixels, size_t count);
DisplaySPI_Status_t DisplaySPI_InitPanel(DisplaySPI_Handle_t *display);
DisplaySPI_Status_t DisplaySPI_FillScreenRGB565(DisplaySPI_Handle_t *display, uint16_t color);

DisplaySPI_Status_t DisplaySPI_SetBacklight(DisplaySPI_Handle_t *display, uint8_t on);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_SPI_H */
