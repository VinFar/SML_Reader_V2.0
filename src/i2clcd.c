#include "functions.h"
#include "i2clcd.h"
#include "defines.h"
#include "stdbool.h"
#include "main.h"
#include "i2c.h"
#include "delay.h"

uint8_t lightOn = 0;
extern double PreiskWhEK, PreiskWhVK, PreiskWhEVsmall, PreiskWhEVbig,
		MonatlGrundpreis;

int32_t rotary_counter;

int32_t *value_ptr;	//pointer to value which has to changed according to rotary encoder
char *string_ptr;

int8_t lcd_poll() {
	uint8_t value = 0;

	if (i2c1_start(0x3f, 1, I2C_READ) < 0) {
		return -1;
	}

	if (i2c1_readNack(&value) < 0) {
		return -1;
	}
	if (i2c1_stop() < 0) {
		return -1;
	}
	return 0;

}

//-	Display initialization sequence

void lcd_init(void) {

//	lcd_light(false);
	lcd_write(CMD_D1 | CMD_D0);	//-	Set interface to 8-bit
	_delay_ms(5);			    //-	Wait for more than 4.1ms
	lcd_write(CMD_D1 | CMD_D0);	//-	Set interface to 8-bit
	_delay_ms(2);		        //-	Wait for more than 100us
	lcd_write(CMD_D1 | CMD_D0);	//-	Set interface to 8-bit
	lcd_write(CMD_D1);		    //-	Set interface to 4-bit
	lcd_write(LCD_CURSOROFF);
	lcd_write(LCD_BLINKINGOFF);

	//- From now on in 4-bit-Mode
	lcd_command(LCD_LINE_MODE | LCD_5X7);
	lcd_command(LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKINGOFF);

//	_delay_ms(2);
	lcd_command(LCD_INCREASE | LCD_DISPLAYSHIFTOFF);
	create_custom_characters();

}

void lcd_clear() {
	lcd_command(LCD_CLEAR);
	_delay_ms(2);
	return;
}

//-	Write data to i2c

void lcd_write_i2c(uint8_t value) {

	if (i2c1_start(0x3f, 1, I2C_WRITE) < 0) {
		I2C1_RESET
		;
		return -1;
	}
	if (i2c1_write(value) < 0) {
		I2C1_RESET
		;
		return -1;
	}
	if (i2c1_stop() < 0) {
		I2C1_RESET
		;
		return -1;
	}
	return;

}

static void lcd_pulseEnable(uint8_t data) {
	lcd_write_i2c(data | LCD_E);	// En high
	delay_us(1);		// enable pulse must be >450ns

	lcd_write_i2c(data & ~LCD_E);	// En low
	delay_us(40);		// commands need > 37us to settle
}

//-	Write nibble to display with pulse of enable bit
// map pinout between PCF8574 and LCD

void lcd_write(uint8_t value) {
	uint8_t data_out = 0;

	// map data to LCD pinout
	if (value & CMD_D0)
		data_out |= LCD_D4;
	if (value & CMD_D1)
		data_out |= LCD_D5;
	if (value & CMD_D2)
		data_out |= LCD_D6;
	if (value & CMD_D3)
		data_out |= LCD_D7;
	if (value & CMD_RS)
		data_out |= LCD_RS;
	if (value & CMD_RW)
		data_out |= LCD_RW;
	if (lightOn)
		data_out |= LCD_LIGHT_N;

	lcd_write_i2c(data_out | LCD_E);		//-	Set new data and enable to high
//	lcd_pulseEnable(data_out);
	delay_us(1);
	lcd_write_i2c(data_out & ~LCD_E);	// En low

}

uint8_t lcd_read(uint8_t mode) {
	uint8_t lcddata, data;

	if (mode == LCD_DATA) {
		lcddata = (LCD_E | LCD_RS | LCD_RW | LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7);
	} else {
		lcddata = (LCD_E | LCD_RW | LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7);
	}

	if (lightOn)
		lcddata |= LCD_LIGHT_N;
	lcd_write_i2c(lcddata);
	lcddata = lcd_read_i2c();

	data = 0;
	// map data from LCD pinout to internal positions
	if (lcddata & LCD_D4)
		data |= CMD_D0;
	if (lcddata & LCD_D5)
		data |= CMD_D1;
	if (lcddata & LCD_D6)
		data |= CMD_D2;
	if (lcddata & LCD_D7)
		data |= CMD_D3;

	lcddata = 0;
	if (lightOn)
		lcddata |= LCD_LIGHT_N;
	lcd_write_i2c(lcddata);

	return data;
}

//-	Read one complete byte via i2c from display

uint8_t lcd_getbyte(uint8_t mode) {
	uint8_t hi, lo;

	hi = lcd_read(mode);
	lo = lcd_read(mode);
	return (hi << 4) + (lo & 0x0F);
}

//-	Issue a command to the display (use the defined commands above)

void lcd_command(uint8_t command) {

	lcd_write((command >> 4));
	lcd_write((command & 0x0F));
}

//-	Print string to cursor position

void lcd_print(char *string) {

	while (*string) {
		lcd_putchar(*string++);
	}
}

//-	Put char to atctual cursor position

void lcd_putchar(char lcddata) {

	lcd_write((lcddata >> 4) | CMD_RS);
	lcd_write((lcddata & 0x0F) | CMD_RS);
}

//-	Put char to position

uint8_t lcd_putcharlc(uint8_t line, uint8_t col, char value) {

	if (!lcd_setcursor(line, col))
		return false;
	lcd_putchar(value);

	return true;
}

//-	Print string to position (If string is longer than LCD_COLS overwrite first chars)(line, row, string)

uint8_t lcd_printlc(uint8_t line, uint8_t col, char *string) {

	if (!lcd_setcursor(line, col))
		return false;

	while (*string) {
		lcd_putchar(*string++);
		col++;
		if (col > LCD_COLS) {
			return true;
		}
	}
	return true;
}

//-	Print string to position (If string is longer than LCD_COLS continue in next line)

uint8_t lcd_printlcc(uint8_t line, uint8_t col, char *string) {

	if (!lcd_setcursor(line, col))
		return false;

	while (*string) {
		lcd_putchar(*string++);
		col++;
		if (col > LCD_COLS) {
			line++;
			col = 1;
			if (line > LCD_LINES) {
				line = 1;
			}
			lcd_setcursor(line, col);
		}
	}
	return true;
}

//-	Go to position (line, column)

uint8_t lcd_setcursor(uint8_t line, uint8_t col) {
	uint8_t lcddata = 0;

	if ((line > LCD_LINES) || (col > LCD_COLS) || ((line == 0) || (col == 0)))
		return false;

	switch (line) {
	case 1:
		lcddata = LCD_LINE1;
		break;
	case 2:
		lcddata = LCD_LINE2;
		break;
	case 3:
		lcddata = LCD_LINE3;
		break;
	case 4:
		lcddata = LCD_LINE4;
		break;
	}
	lcddata |= 0x80;
	lcddata += (col - 1);
	lcd_command(lcddata);
	return true;
}

//-	Go to nextline (if next line > (LCD_LINES-1) return false)

uint8_t lcd_nextline(void) {
	uint8_t line, col;

	lcd_getlc(&line, &col);
	if (!lcd_setcursor(line + 1, 1)) {
		return false;
	} else {
		return true;
	}
}

//-	Get line and row (target byte for line, target byte for row)

uint8_t lcd_getlc(uint8_t *line, uint8_t *col) {
	uint8_t lcddata;

	lcddata = lcd_getbyte(LCD_ADDRESS);
	if (lcddata & (1 << 7))
		return false;       // LCD busy

	if (lcddata >= LCD_LINE1 && lcddata < (LCD_LINE1 + LCD_COLS)) {
		*line = 1;
		*col = lcddata - LCD_LINE1 + 1;
		return true;
	} else if (lcddata >= LCD_LINE2 && lcddata < (LCD_LINE2 + LCD_COLS)) {
		*line = 2;
		*col = lcddata - LCD_LINE2 + 1;
		return true;
	} else if (lcddata >= LCD_LINE3 && lcddata < (LCD_LINE3 + LCD_COLS)) {
		*line = 3;
		*col = lcddata - LCD_LINE3 + 1;
		return true;
	} else if (lcddata >= LCD_LINE4 && lcddata < (LCD_LINE4 + LCD_COLS)) {
		*line = 4;
		*col = lcddata - LCD_LINE4 + 1;
		return true;
	}

	return false;
}

// turn light on/off

void lcd_light(uint8_t light) {
	if (light) {
		lcd_write_i2c(LCD_LIGHT_ON);
	}
}

//-	Check if busy

uint8_t lcd_busy(void) {
	uint8_t state;

	state = lcd_getbyte(LCD_ADDRESS);
	if (state & (1 << 7)) {
		return true;
	} else {
		return false;
	}
}

void lcd_printint(int32_t data) {

	char tmp[32] = "";
	itoa(data, tmp, sizeof(tmp));
	lcd_print(tmp);

	return;
}

void lcd_printarrow(int line) {

	if (line > 4) {
		line = 4;
	}
	if (line < 1) {
		line = 1;
	}

	lcd_setcursor(old_arrow_line, 1);
	lcd_putchar(' ');
	lcd_setcursor(line, 1);
	lcd_putchar('>');
	old_arrow_line = line;
	return;

}

void lcd_print_value_unit(int pos_line, int pos_row, char *value, char *unit) {

	for (char i = pos_row; i <= 20; i++) {		//flush line
		lcd_setcursor(pos_line, i);
		lcd_putchar(' ');
	}
	lcd_setcursor(pos_line, pos_row);
	lcd_print(value);
	lcd_print(unit);

}

static int lcd_fwrite(int file, char *ptr, int len) {
	int DataIdx;

	for (DataIdx = 0; DataIdx < len; DataIdx++) {
		lcd_putchar(*ptr++);
	}
	return len;
}

int8_t lcd_printf(const char *fmt, ...) {
	int length = 0;
	va_list va;
	va_start(va, fmt);
	length = ts_formatlength(fmt, va);
	va_end(va);
	{
		char buf[length];
		va_start(va, fmt);
		length = ts_formatstring(buf, fmt, va);
		length = lcd_fwrite(1, buf, length);
		va_end(va);
	}
	return length;
}

void lcd_create_char(uint8_t location, uint8_t charmap[]) {
	location &= 0x7; // we only have 8 locations 0-7
	lcd_command(LCD_SETCGRAMADDR | (location << 3));
	for (int i = 0; i < 8; i++) {
		lcd_putchar(charmap[i]);
	}
}

uint8_t verticalLine[8] = { 0b00100, 0b00100, 0b00100, 0b00100, 0b00100,
		0b00100, 0b00100, 0b00100 };

uint8_t char2[8] = { 0b00000, 0b00000, 0b00000, 0b11100, 0b00100, 0b00100,
		0b00100, 0b00100 };

uint8_t char1[8] = { 0b00000, 0b00000, 0b00000, 0b00111, 0b00100, 0b00100,
		0b00100, 0b00100 };

uint8_t char3[8] = { 0b00100, 0b00100, 0b00100, 0b00111, 0b00000, 0b00000,
		0b00000, 0b00000 };

uint8_t char4[8] = { 0b00100, 0b00100, 0b00100, 0b11100, 0b00000, 0b00000,
		0b00000, 0b00000 };

void lcd_print_frame() {
	lcd_setcursor(1, 2);
	lcd_print("------------------");
	lcd_setcursor(4, 2);
	lcd_print("------------------");
	lcd_setcursor(2, 1);
	lcd_putchar((0));
	lcd_setcursor(3, 1);
	lcd_putchar((0));
	lcd_setcursor(2, 20);
	lcd_putchar((0));
	lcd_setcursor(3, 20);
	lcd_putchar((0));
	lcd_setcursor(1, 1);
	lcd_putchar((1));
	lcd_setcursor(1, 20);
	lcd_putchar((2));
	lcd_setcursor(4, 1);
	lcd_putchar((3));
	lcd_setcursor(4, 20);
	lcd_putchar((4));
}

void create_custom_characters() {
	lcd_create_char(0, verticalLine);
	lcd_create_char(1, char1);
	lcd_create_char(2, char2);
	lcd_create_char(3, char3);
	lcd_create_char(4, char4);
}
