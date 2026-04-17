#include "LCD.h"

#include "spi.h"
#include <string.h>

#include "../../Ref/FONT.H"

uint16_t POINT_COLOR = BLACK;
uint16_t BACK_COLOR = WHITE;
lcd_dev_t lcddev;

static inline void lcd_cs_set(GPIO_PinState state)
{
	HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, state);
}

static inline void lcd_dc_set(GPIO_PinState state)
{
	HAL_GPIO_WritePin(SPI1_DC_GPIO_Port, SPI1_DC_Pin, state);
}

static inline void lcd_rst_set(GPIO_PinState state)
{
	HAL_GPIO_WritePin(SPI1_RST_GPIO_Port, SPI1_RST_Pin, state);
}

static void lcd_tx(const uint8_t *data, uint16_t len)
{
	(void)HAL_SPI_Transmit(&hspi1, (uint8_t *)data, len, 100U);
}

static void LCD_WR_REG(uint8_t reg)
{
	lcd_cs_set(GPIO_PIN_RESET);
	lcd_dc_set(GPIO_PIN_RESET);
	lcd_tx(&reg, 1U);
	lcd_cs_set(GPIO_PIN_SET);
}

static void LCD_WR_DATA8(uint8_t data)
{
	lcd_cs_set(GPIO_PIN_RESET);
	lcd_dc_set(GPIO_PIN_SET);
	lcd_tx(&data, 1U);
	lcd_cs_set(GPIO_PIN_SET);
}

static void LCD_WR_DATA16(uint16_t data)
{
	uint8_t tx[2];
	tx[0] = (uint8_t)(data >> 8);
	tx[1] = (uint8_t)(data & 0xFFU);

	lcd_cs_set(GPIO_PIN_RESET);
	lcd_dc_set(GPIO_PIN_SET);
	lcd_tx(tx, 2U);
	lcd_cs_set(GPIO_PIN_SET);
}

static void LCD_WriteCommandData(const uint8_t *cmdData, uint16_t len)
{
	if (len == 0U)
	{
		return;
	}

	lcd_cs_set(GPIO_PIN_RESET);
	lcd_dc_set(GPIO_PIN_SET);
	lcd_tx(cmdData, len);
	lcd_cs_set(GPIO_PIN_SET);
}

static void LCD_WriteMemoryRepeated(uint16_t color, uint32_t count)
{
	uint8_t pair[2];
	pair[0] = (uint8_t)(color >> 8);
	pair[1] = (uint8_t)(color & 0xFFU);

	lcd_cs_set(GPIO_PIN_RESET);
	lcd_dc_set(GPIO_PIN_SET);
	while (count-- > 0U)
	{
		lcd_tx(pair, 2U);
	}
	lcd_cs_set(GPIO_PIN_SET);
}

static uint32_t LCD_Pow(uint8_t m, uint8_t n)
{
	uint32_t result = 1U;
	while (n-- > 0U)
	{
		result *= m;
	}
	return result;
}

void LCD_WriteReg(uint16_t reg, uint16_t value)
{
	LCD_WR_REG((uint8_t)reg);
	LCD_WR_DATA16(value);
}

void LCD_WriteRAM_Prepare(void)
{
	LCD_WR_REG((uint8_t)lcddev.wramcmd);
}

void LCD_WriteRAM(uint16_t rgb)
{
	LCD_WR_DATA16(rgb);
}

void LCD_DisplayOn(void)
{
	LCD_WR_REG(0x29U);
}

void LCD_DisplayOff(void)
{
	LCD_WR_REG(0x28U);
}

void LCD_SetCursor(uint16_t x, uint16_t y)
{
	uint8_t data[4];

	LCD_WR_REG((uint8_t)lcddev.setxcmd);
	data[0] = (uint8_t)(x >> 8);
	data[1] = (uint8_t)(x & 0xFFU);
	data[2] = data[0];
	data[3] = data[1];
	LCD_WriteCommandData(data, 4U);

	LCD_WR_REG((uint8_t)lcddev.setycmd);
	data[0] = (uint8_t)(y >> 8);
	data[1] = (uint8_t)(y & 0xFFU);
	data[2] = data[0];
	data[3] = data[1];
	LCD_WriteCommandData(data, 4U);
}

void LCD_DrawPoint(uint16_t x, uint16_t y)
{
	LCD_SetCursor(x, y);
	LCD_WriteRAM_Prepare();
	LCD_WriteRAM(POINT_COLOR);
}

void LCD_Fast_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
	LCD_SetCursor(x, y);
	LCD_WriteRAM_Prepare();
	LCD_WriteRAM(color);
}

void LCD_Display_Dir(uint8_t dir)
{
	if ((dir == 0U) || (dir == 1U))
	{
		lcddev.dir = 0U;
		lcddev.width = 240U;
		lcddev.height = 320U;
		if (dir == 0U)
		{
			LCD_WR_REG(0x36U);
			LCD_WR_DATA8((uint8_t)((1U << 3) | (1U << 6)));
		}
		else
		{
			LCD_WR_REG(0x36U);
			LCD_WR_DATA8((uint8_t)((1U << 3) | (1U << 7)));
		}
	}
	else
	{
		lcddev.dir = 1U;
		lcddev.width = 320U;
		lcddev.height = 240U;
		if (dir == 2U)
		{
			LCD_WR_REG(0x36U);
			LCD_WR_DATA8((uint8_t)((1U << 3) | (1U << 7) | (1U << 6) | (1U << 5)));
		}
		else
		{
			LCD_WR_REG(0x36U);
			LCD_WR_DATA8((uint8_t)((1U << 3) | (1U << 5)));
		}
	}

	lcddev.wramcmd = 0x2CU;
	lcddev.setxcmd = 0x2AU;
	lcddev.setycmd = 0x2BU;
	LCD_Set_Window(0U, 0U, lcddev.width, lcddev.height);
}

void LCD_Set_Window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
	uint16_t ex = (uint16_t)(sx + width - 1U);
	uint16_t ey = (uint16_t)(sy + height - 1U);
	uint8_t data[4];

	LCD_WR_REG((uint8_t)lcddev.setxcmd);
	data[0] = (uint8_t)(sx >> 8);
	data[1] = (uint8_t)(sx & 0xFFU);
	data[2] = (uint8_t)(ex >> 8);
	data[3] = (uint8_t)(ex & 0xFFU);
	LCD_WriteCommandData(data, 4U);

	LCD_WR_REG((uint8_t)lcddev.setycmd);
	data[0] = (uint8_t)(sy >> 8);
	data[1] = (uint8_t)(sy & 0xFFU);
	data[2] = (uint8_t)(ey >> 8);
	data[3] = (uint8_t)(ey & 0xFFU);
	LCD_WriteCommandData(data, 4U);
}

void LCD_Init(void)
{
	lcd_rst_set(GPIO_PIN_SET);
	HAL_Delay(1U);
	lcd_rst_set(GPIO_PIN_RESET);
	HAL_Delay(10U);
	lcd_rst_set(GPIO_PIN_SET);
	HAL_Delay(120U);

	LCD_WR_REG(0xCFU); LCD_WR_DATA8(0x00U); LCD_WR_DATA8(0x89U); LCD_WR_DATA8(0x30U);
	LCD_WR_REG(0xEDU); LCD_WR_DATA8(0x67U); LCD_WR_DATA8(0x03U); LCD_WR_DATA8(0x12U); LCD_WR_DATA8(0x81U);
	LCD_WR_REG(0xE8U); LCD_WR_DATA8(0x85U); LCD_WR_DATA8(0x01U); LCD_WR_DATA8(0x78U);
	LCD_WR_REG(0xCBU); LCD_WR_DATA8(0x39U); LCD_WR_DATA8(0x2CU); LCD_WR_DATA8(0x00U); LCD_WR_DATA8(0x34U); LCD_WR_DATA8(0x02U);
	LCD_WR_REG(0xF7U); LCD_WR_DATA8(0x20U);
	LCD_WR_REG(0xEAU); LCD_WR_DATA8(0x00U); LCD_WR_DATA8(0x00U);
	LCD_WR_REG(0xC0U); LCD_WR_DATA8(0x25U);
	LCD_WR_REG(0xC1U); LCD_WR_DATA8(0x10U);
	LCD_WR_REG(0xC5U); LCD_WR_DATA8(0x49U); LCD_WR_DATA8(0x4FU);
	LCD_WR_REG(0xC7U); LCD_WR_DATA8(0xB0U);
	LCD_WR_REG(0xB6U); LCD_WR_DATA8(0x0AU); LCD_WR_DATA8(0x82U);
	LCD_WR_REG(0x36U); LCD_WR_DATA8(0x48U);
	LCD_WR_REG(0x3AU); LCD_WR_DATA8(0x55U);
	LCD_WR_REG(0xF2U); LCD_WR_DATA8(0x00U);
	LCD_WR_REG(0x26U); LCD_WR_DATA8(0x01U);

	LCD_WR_REG(0xE0U);
	LCD_WR_DATA8(0x0FU); LCD_WR_DATA8(0x27U); LCD_WR_DATA8(0x23U); LCD_WR_DATA8(0x0BU); LCD_WR_DATA8(0x0FU);
	LCD_WR_DATA8(0x05U); LCD_WR_DATA8(0x54U); LCD_WR_DATA8(0x74U); LCD_WR_DATA8(0x45U); LCD_WR_DATA8(0x0AU);
	LCD_WR_DATA8(0x17U); LCD_WR_DATA8(0x0AU); LCD_WR_DATA8(0x1CU); LCD_WR_DATA8(0x0EU); LCD_WR_DATA8(0x08U);

	LCD_WR_REG(0xE1U);
	LCD_WR_DATA8(0x08U); LCD_WR_DATA8(0x1AU); LCD_WR_DATA8(0x1EU); LCD_WR_DATA8(0x03U); LCD_WR_DATA8(0x0FU);
	LCD_WR_DATA8(0x05U); LCD_WR_DATA8(0x2EU); LCD_WR_DATA8(0x25U); LCD_WR_DATA8(0x3BU); LCD_WR_DATA8(0x01U);
	LCD_WR_DATA8(0x06U); LCD_WR_DATA8(0x05U); LCD_WR_DATA8(0x25U); LCD_WR_DATA8(0x33U); LCD_WR_DATA8(0x0FU);

	LCD_WR_REG(0x21U);
	LCD_WR_REG(0x11U);
	HAL_Delay(120U);
	LCD_WR_REG(0x29U);

	lcddev.id = 0x9341U;
	LCD_Display_Dir(USE_LCM_DIR);
}

uint16_t LCD_ReadPoint(uint16_t x, uint16_t y)
{
	(void)x;
	(void)y;
	/* Readback is not wired in current SPI setup (MISO not configured for LCD read). */
	return 0U;
}

void LCD_Clear(uint16_t color)
{
	LCD_Set_Window(0U, 0U, lcddev.width, lcddev.height);
	LCD_WriteRAM_Prepare();
	LCD_WriteMemoryRepeated(color, (uint32_t)lcddev.width * (uint32_t)lcddev.height);
}

void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
	uint32_t pixels;

	if ((ex < sx) || (ey < sy))
	{
		return;
	}

	LCD_Set_Window(sx, sy, (uint16_t)(ex - sx + 1U), (uint16_t)(ey - sy + 1U));
	LCD_WriteRAM_Prepare();
	pixels = (uint32_t)(ex - sx + 1U) * (uint32_t)(ey - sy + 1U);
	LCD_WriteMemoryRepeated(color, pixels);
	LCD_Set_Window(0U, 0U, lcddev.width, lcddev.height);
}

void LCD_Color_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
	uint32_t i;
	uint32_t count;

	if ((color == NULL) || (ex < sx) || (ey < sy))
	{
		return;
	}

	LCD_Set_Window(sx, sy, (uint16_t)(ex - sx + 1U), (uint16_t)(ey - sy + 1U));
	LCD_WriteRAM_Prepare();
	count = (uint32_t)(ex - sx + 1U) * (uint32_t)(ey - sy + 1U);
	for (i = 0U; i < count; i++)
	{
		LCD_WriteRAM(color[i]);
	}
	LCD_Set_Window(0U, 0U, lcddev.width, lcddev.height);
}

void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	int xerr = 0;
	int yerr = 0;
	int delta_x = (int)x2 - (int)x1;
	int delta_y = (int)y2 - (int)y1;
	int incx;
	int incy;
	int distance;
	int x = (int)x1;
	int y = (int)y1;
	uint16_t t;

	if (delta_x > 0) { incx = 1; }
	else if (delta_x == 0) { incx = 0; }
	else { incx = -1; delta_x = -delta_x; }

	if (delta_y > 0) { incy = 1; }
	else if (delta_y == 0) { incy = 0; }
	else { incy = -1; delta_y = -delta_y; }

	distance = (delta_x > delta_y) ? delta_x : delta_y;
	for (t = 0U; t <= (uint16_t)(distance + 1); t++)
	{
		LCD_DrawPoint((uint16_t)x, (uint16_t)y);
		xerr += delta_x;
		yerr += delta_y;
		if (xerr > distance) { xerr -= distance; x += incx; }
		if (yerr > distance) { yerr -= distance; y += incy; }
	}
}

void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	LCD_DrawLine(x1, y1, x2, y1);
	LCD_DrawLine(x1, y1, x1, y2);
	LCD_DrawLine(x1, y2, x2, y2);
	LCD_DrawLine(x2, y1, x2, y2);
}

void LCD_Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r)
{
	int a = 0;
	int b = (int)r;
	int di = 3 - ((int)r << 1);

	while (a <= b)
	{
		LCD_DrawPoint((uint16_t)(x0 + a), (uint16_t)(y0 - b));
		LCD_DrawPoint((uint16_t)(x0 + b), (uint16_t)(y0 - a));
		LCD_DrawPoint((uint16_t)(x0 + b), (uint16_t)(y0 + a));
		LCD_DrawPoint((uint16_t)(x0 + a), (uint16_t)(y0 + b));
		LCD_DrawPoint((uint16_t)(x0 - a), (uint16_t)(y0 + b));
		LCD_DrawPoint((uint16_t)(x0 - b), (uint16_t)(y0 + a));
		LCD_DrawPoint((uint16_t)(x0 - a), (uint16_t)(y0 - b));
		LCD_DrawPoint((uint16_t)(x0 - b), (uint16_t)(y0 - a));
		a++;
		if (di < 0) { di += (4 * a + 6); }
		else { di += 10 + 4 * (a - b); b--; }
	}
}

void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint8_t size, uint8_t mode)
{
	uint8_t temp;
	uint8_t t1;
	uint8_t t;
	uint16_t y0 = y;
	uint8_t csize = (uint8_t)((size / 8U + ((size % 8U) ? 1U : 0U)) * (size / 2U));
	const unsigned char *fontPtr = NULL;

	if ((num < ' ') || (num > '~'))
	{
		return;
	}

	num = (uint8_t)(num - ' ');
	for (t = 0U; t < csize; t++)
	{
		if (size == 12U) { fontPtr = &asc2_1206[num][t]; }
		else if (size == 16U) { fontPtr = &asc2_1608[num][t]; }
		else if (size == 24U) { fontPtr = &asc2_2412[num][t]; }
		else { return; }

		temp = *fontPtr;
		for (t1 = 0U; t1 < 8U; t1++)
		{
			if ((temp & 0x80U) != 0U) { LCD_Fast_DrawPoint(x, y, POINT_COLOR); }
			else if (mode == 0U) { LCD_Fast_DrawPoint(x, y, BACK_COLOR); }
			temp <<= 1;
			y++;
			if (y >= lcddev.height) { return; }
			if ((y - y0) == size)
			{
				y = y0;
				x++;
				if (x >= lcddev.width) { return; }
				break;
			}
		}
	}
}

void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size)
{
	uint8_t t;
	uint8_t temp;
	uint8_t enshow = 0U;

	for (t = 0U; t < len; t++)
	{
		temp = (uint8_t)((num / LCD_Pow(10U, (uint8_t)(len - t - 1U))) % 10U);
		if ((enshow == 0U) && (t < (uint8_t)(len - 1U)))
		{
			if (temp == 0U)
			{
				LCD_ShowChar((uint16_t)(x + (size / 2U) * t), y, ' ', size, 0U);
				continue;
			}
			enshow = 1U;
		}
		LCD_ShowChar((uint16_t)(x + (size / 2U) * t), y, (uint8_t)(temp + '0'), size, 0U);
	}
}

void LCD_ShowxNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode)
{
	uint8_t t;
	uint8_t temp;
	uint8_t enshow = 0U;

	for (t = 0U; t < len; t++)
	{
		temp = (uint8_t)((num / LCD_Pow(10U, (uint8_t)(len - t - 1U))) % 10U);
		if ((enshow == 0U) && (t < (uint8_t)(len - 1U)))
		{
			if (temp == 0U)
			{
				LCD_ShowChar((uint16_t)(x + (size / 2U) * t), y, (mode & 0x80U) ? '0' : ' ', size, (uint8_t)(mode & 0x01U));
				continue;
			}
			enshow = 1U;
		}
		LCD_ShowChar((uint16_t)(x + (size / 2U) * t), y, (uint8_t)(temp + '0'), size, (uint8_t)(mode & 0x01U));
	}
}

void LCD_ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, uint8_t *p)
{
	uint16_t x0 = x;
	uint16_t xEnd = (uint16_t)(x + width);
	uint16_t yEnd = (uint16_t)(y + height);

	if (p == NULL)
	{
		return;
	}

	while ((*p <= '~') && (*p >= ' '))
	{
		if (x >= xEnd) { x = x0; y = (uint16_t)(y + size); }
		if (y >= yEnd) { break; }
		LCD_ShowChar(x, y, *p, size, 0U);
		x = (uint16_t)(x + size / 2U);
		p++;
	}
}

void Show_Str(uint16_t x, uint16_t y, uint8_t *str, uint8_t size, uint8_t mode)
{
	/* ASCII-only path for this HAL port. */
	(void)mode;
	LCD_ShowString(x, y, lcddev.width - x, lcddev.height - y, size, str);
}

void Gui_Drawbmp16(uint16_t x, uint16_t y, const unsigned char *p)
{
	uint32_t i;
	if (p == NULL)
	{
		return;
	}

	LCD_Set_Window(x, y, 40U, 40U);
	LCD_WriteRAM_Prepare();
	for (i = 0U; i < (40U * 40U); i++)
	{
		uint16_t c = (uint16_t)(((uint16_t)p[i * 2U + 1U] << 8) | p[i * 2U]);
		LCD_WriteRAM(c);
	}
	LCD_Set_Window(0U, 0U, lcddev.width, lcddev.height);
}

void Gui_StrCenter(uint16_t x, uint16_t y, uint8_t *str, uint8_t size, uint8_t mode)
{
	uint16_t x1;
	uint16_t len = (uint16_t)strlen((const char *)str);

	if (size > 16U) { x1 = (uint16_t)((lcddev.width - len * (size / 2U)) / 2U); }
	else { x1 = (uint16_t)((lcddev.width - len * 8U) / 2U); }
	Show_Str((uint16_t)(x + x1), y, str, size, mode);
}

void Load_Drow_Dialog(void)
{
	LCD_Clear(WHITE);
	POINT_COLOR = BLUE;
	BACK_COLOR = WHITE;
	LCD_ShowString((uint16_t)(lcddev.width - 24U), 0U, 200U, 16U, 16U, (uint8_t *)"RST");
	POINT_COLOR = RED;
}

void gui_draw_hline(uint16_t x0, uint16_t y0, uint16_t len, uint16_t color)
{
	if (len == 0U)
	{
		return;
	}
	LCD_Fill(x0, y0, (uint16_t)(x0 + len - 1U), y0, color);
}

void gui_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
	uint32_t i;
	uint32_t imax = ((uint32_t)r * 707U) / 1000U + 1U;
	uint32_t sqmax = (uint32_t)r * (uint32_t)r + (uint32_t)r / 2U;
	uint32_t x = r;

	gui_draw_hline((uint16_t)(x0 - r), y0, (uint16_t)(2U * r), color);
	for (i = 1U; i <= imax; i++)
	{
		if ((i * i + x * x) > sqmax)
		{
			if (x > imax)
			{
				gui_draw_hline((uint16_t)(x0 - i + 1U), (uint16_t)(y0 + x), (uint16_t)(2U * (i - 1U)), color);
				gui_draw_hline((uint16_t)(x0 - i + 1U), (uint16_t)(y0 - x), (uint16_t)(2U * (i - 1U)), color);
			}
			x--;
		}
		gui_draw_hline((uint16_t)(x0 - x), (uint16_t)(y0 + i), (uint16_t)(2U * x), color);
		gui_draw_hline((uint16_t)(x0 - x), (uint16_t)(y0 - i), (uint16_t)(2U * x), color);
	}
}

void lcd_draw_bline(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t size, uint16_t color)
{
	uint16_t t;
	int xerr = 0;
	int yerr = 0;
	int delta_x;
	int delta_y;
	int distance;
	int incx;
	int incy;
	int uRow;
	int uCol;

	if ((x1 < size) || (x2 < size) || (y1 < size) || (y2 < size))
	{
		return;
	}

	delta_x = (int)x2 - (int)x1;
	delta_y = (int)y2 - (int)y1;
	uRow = x1;
	uCol = y1;

	if (delta_x > 0) { incx = 1; }
	else if (delta_x == 0) { incx = 0; }
	else { incx = -1; delta_x = -delta_x; }

	if (delta_y > 0) { incy = 1; }
	else if (delta_y == 0) { incy = 0; }
	else { incy = -1; delta_y = -delta_y; }

	distance = (delta_x > delta_y) ? delta_x : delta_y;
	for (t = 0U; t <= (uint16_t)(distance + 1); t++)
	{
		gui_fill_circle((uint16_t)uRow, (uint16_t)uCol, size, color);
		xerr += delta_x;
		yerr += delta_y;
		if (xerr > distance) { xerr -= distance; uRow += incx; }
		if (yerr > distance) { yerr -= distance; uCol += incy; }
	}
}

