/*
 * HAL-based LCD driver interface adapted from Ref/LCD.h
 */
#ifndef __LCD_H
#define __LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "main.h"

/* Rotation options: 0=0deg, 1=180deg, 2=270deg, 3=90deg */
#define USE_LCM_DIR 0U

/* LCD parameters */
typedef struct
{
	uint16_t width;
	uint16_t height;
	uint16_t id;
	uint8_t dir;
	uint16_t wramcmd;
	uint16_t setxcmd;
	uint16_t setycmd;
} lcd_dev_t;

extern lcd_dev_t lcddev;
extern uint16_t POINT_COLOR;
extern uint16_t BACK_COLOR;

/* Color definitions (RGB565) */
#define WHITE      0xFFFFU
#define BLACK      0x0000U
#define BLUE       0x001FU
#define BRED       0xF81FU
#define GRED       0xFFE0U
#define GBLUE      0x07FFU
#define RED        0xF800U
#define MAGENTA    0xF81FU
#define GREEN      0x07E0U
#define CYAN       0x7FFFU
#define YELLOW     0xFFE0U
#define BROWN      0xBC40U
#define BRRED      0xFC07U
#define GRAY       0x8430U

#define DARKBLUE   0x01CFU
#define LIGHTBLUE  0x7D7CU
#define GRAYBLUE   0x5458U

#define LIGHTGREEN 0x841FU
#define LGRAY      0xC618U

#define LGRAYBLUE  0xA651U
#define LBBLUE     0x2B12U

void LCD_Init(void);
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
void LCD_Clear(uint16_t color);
void LCD_SetCursor(uint16_t x, uint16_t y);
void LCD_DrawPoint(uint16_t x, uint16_t y);
void LCD_Fast_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void LCD_Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color);
void LCD_Color_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);
void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint8_t size, uint8_t mode);
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size);
void LCD_ShowxNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode);
void LCD_ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, uint8_t *p);

void LCD_WriteReg(uint16_t reg, uint16_t value);
void LCD_WriteRAM_Prepare(void);
void LCD_WriteRAM(uint16_t rgb);
void Load_Drow_Dialog(void);
void LCD_Set_Window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);
void Show_Str(uint16_t x, uint16_t y, uint8_t *str, uint8_t size, uint8_t mode);
void Gui_Drawbmp16(uint16_t x, uint16_t y, const unsigned char *p);
void Gui_StrCenter(uint16_t x, uint16_t y, uint8_t *str, uint8_t size, uint8_t mode);
void LCD_Display_Dir(uint8_t dir);
void lcd_draw_bline(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t size, uint16_t color);
void gui_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void gui_draw_hline(uint16_t x0, uint16_t y0, uint16_t len, uint16_t color);
uint16_t LCD_ReadPoint(uint16_t x, uint16_t y);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_H */
