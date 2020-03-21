#ifndef _I2CLCD_H
#define _I2CLCD_H

#include "defines.h"
#include "main.h"
#include "lcd_menu.h"


//extern char* menu_items[37];
extern int32_t* value_ptr;	//pointer to value which has to changed according to rotary encoder
extern int32_t dummy_value;
extern char* string_ptr;
extern char dummy_string;
extern uint32_t jog_value;
extern char PARITY_array[3][3];
extern char BAUDRATE_array[2][6];
extern uint8_t value_item_counter;
extern uint8_t old_arrow_line;
extern char tmp, ctr2, ctr1;



#define LCD_I2C_DEVICE		0x3F	    /**< Change this to the address of your expander */
#define LCD_LINES			4	        /**< Enter the number of lines of your display here */
#define LCD_COLS			20	        /**< Enter the number of columns of your display here */
#define LCD_LINE_MODE       LCD_4LINE   /**< Enter line mode your display here */

#define LCD_LINE1			0x00	    /**< This should be 0x00 on all displays */
#define LCD_LINE2			0x40	    /**< Change this to the address for line 2 on your display */
#define LCD_LINE3			0x14	    /**< Change this to the address for line 3 on your display */
#define LCD_LINE4			0x54	    /**< Change this to the address for line 4 on your display */

#define BLOCK 				0xFF
#define CHECKBOX			"<"
/*@}*/

#if LCD_LINES > 4
#error "#define LCD_LINES must be less or equal to 4"
#endif

#if LCD_COLS > 20
#error "#define LCD_COLS must be less or equal to 20"
#endif

//-------------------------------------------------------------------------------------------------------------------

//--The-following-definitions-are-corresponding-to-the-PIN-Assignment-(see-above)------------------------------------

/** \defgroup PIN_ASSIGNMENT PIN ASSIGNMENT
 This pin assignment shows how the display is connected to the PCF8574.
 Set the definition to match your hardware setup. Any assignment is possible, but avoid mapping of two signal to one pin!
 */
/*@{*/
#define LCD_D4_PIN			4	/**< LCD-Pin D4 is connected to P4 on the PCF8574 */
#define LCD_D5_PIN			5	/**< LCD-Pin D5 is connected to P5 on the PCF8574 */
#define LCD_D6_PIN			6	/**< LCD-Pin D6 is connected to P6 on the PCF8574 */
#define LCD_D7_PIN			7	/**< LCD-Pin D7 is connected to P7 on the PCF8574 */
#define LCD_RS_PIN			0	/**< LCD-Pin RS is connected to P0 on the PCF8574 */
#define LCD_RW_PIN			1	/**< LCD-Pin RW is connected to P1 on the PCF8574 */
#define LCD_E_PIN			  2	/**< LCD-Pin E is connected to P2 on the PCF8574 */
#define LCD_LIGHT_PIN		3	/**< LCD backlight is connected to P3 on the PCF8574, low active */
/*@}*/

//-------------------------------------------------------------------------------------------------------------------
/** \defgroup DEFINED_BITS DEFINED BITS
 With each read/write operation to/from the display one bytes is send/received. \n
 It contains the control bits RS, RW, LIGHT_N and ENABLE and four data bits.
 */
/*@{*/

#define LCD_D4				(1 << LCD_D4_PIN)	/**< bit 4 in 2nd lower nibble */
#define LCD_D5				(1 << LCD_D5_PIN)	/**< bit 5 in 2nd lower nibble */
#define LCD_D6				(1 << LCD_D6_PIN)	/**< bit 6 in 2nd lower nibble */
#define LCD_D7				(1 << LCD_D7_PIN)	/**< bit 7 in 2nd lower nibble */

#define LCD_RS				(1 << LCD_RS_PIN)	/**< RS-bit in 1st and 2nd higher nibble */
#define LCD_RW				(1 << LCD_RW_PIN)	/**< RW-bit in 1st and 2nd higher nibble */
#define LCD_LIGHT_N			(1 << LCD_LIGHT_PIN)/**< LCD backlight control, low active */
#define LCD_E				(1 << LCD_E_PIN)	/**< E-bit in 1st and 2nd higher nibble */

/*@}*/

// data & control bits for internal use, do not change!
#define CMD_D0				(1 << 0)	/**< bit 0 in lower nibble */
#define CMD_D1				(1 << 1)	/**< bit 1 in lower nibble */
#define CMD_D2				(1 << 2)	/**< bit 2 in lower nibble */
#define CMD_D3				(1 << 3)	/**< bit 3 in lower nibble */
#define CMD_RS				(1 << 4)	/**< RS-bit */
#define CMD_RW				(1 << 5)	/**< RW-bit */

/** \defgroup DEFINED_READ_MODES DEFINED READ MODES
 */
/*@{*/
#define LCD_ADDRESS			0	/**< Used for reading the address-counter and busy-flag */
#define LCD_DATA			1	/**< Used for reading data */
/*@}*/

//-LCD-COMMANDS------------------------------------------------------------------------------------------------------
/** \defgroup DEFINED_COMMANDS DEFINED COMMANDS
 These defined commands should be used to configure the display. \n
 Don't use commands from different categories together. \n
 
 Configuration commands from one category should get combined to one command.
 \par Example: 
 \code lcd_command(LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKINGON); \endcode
 
 The category modes like LCD_SHIFTMODE and LCD_CONFIGURATION can be omitted.
 */
/*@{*/

/** @name GENERAL COMMANDS */
/*@{*/
#define LCD_CLEAR			0x01	/**< Clear screen */
#define LCD_HOME			0x02	/**< Cursor move to first digit */
/*@}*/

/** @name ENTRYMODES */
/*@{*/
#define LCD_ENTRYMODE			0x04			/**< Set entrymode */
#define LCD_INCREASE		LCD_ENTRYMODE | 0x02	/**<	Set cursor move direction -- Increase */
#define LCD_DECREASE		LCD_ENTRYMODE | 0x00	/**<	Set cursor move direction -- Decrease */
#define LCD_DISPLAYSHIFTON	LCD_ENTRYMODE | 0x01	/**<	Display is shifted */
#define LCD_DISPLAYSHIFTOFF	LCD_ENTRYMODE | 0x00	/**<	Display is not shifted */
/*@}*/

/** @name DISPLAYMODES */
/*@{*/
#define LCD_DISPLAYMODE			0x08			/**< Set displaymode */
#define LCD_DISPLAYON		LCD_DISPLAYMODE | 0x04	/**<	Display on */
#define LCD_DISPLAYOFF		LCD_DISPLAYMODE | 0x00	/**<	Display off */
#define LCD_CURSORON		LCD_DISPLAYMODE | 0x02	/**<	Cursor on */
#define LCD_CURSOROFF		LCD_DISPLAYMODE | 0x00	/**<	Cursor off */
#define LCD_BLINKINGON		LCD_DISPLAYMODE | 0x01	/**<	Blinking on */
#define LCD_BLINKINGOFF		LCD_DISPLAYMODE | 0x00	/**<	Blinking off */
/*@}*/

/** @name SHIFTMODES */
/*@{*/
#define LCD_SHIFTMODE			0x10			/**< Set shiftmode */
#define LCD_DISPLAYSHIFT	LCD_SHIFTMODE | 0x08	/**<	Display shift */
#define LCD_CURSORMOVE		LCD_SHIFTMODE | 0x00	/**<	Cursor move */
#define LCD_RIGHT			LCD_SHIFTMODE | 0x04	/**<	Right shift */
#define LCD_LEFT			LCD_SHIFTMODE | 0x00	/**<	Left shift */
/*@}*/

/** @name DISPLAY_CONFIGURATION */
/*@{*/
#define LCD_CONFIGURATION		0x20				/**< Set function */
#define LCD_8BIT		LCD_CONFIGURATION | 0x10	/**<	8 bits interface */
#define LCD_4BIT		LCD_CONFIGURATION | 0x00	/**<	4 bits interface */
#define LCD_4LINE						    0x09	/**<	4 line display */
#define LCD_2LINE		LCD_CONFIGURATION | 0x08	/**<	2 line display */
#define LCD_1LINE		LCD_CONFIGURATION | 0x00	/**<	1 line display */
#define LCD_5X10		LCD_CONFIGURATION | 0x04	/**<	5 X 10 dots */
#define LCD_5X7			LCD_CONFIGURATION | 0x00	/**<	5 X 7 dots */

#define LCD_LIGHT_OFF		LCD_LIGHT_N       // low active
#define LCD_LIGHT_ON		0x00
/*@}*/

//-------------------------------------------------------------------------------------------------------------------
/*@}*/

//-FUNCTIONS---------------------------------------------------------------------------------------------------------
/** \defgroup FUNCTIONS_INTERNAL INTERNAL FUNCTIONS */
/*@{*/

double PreiskWhEK, PreiskWhVK, PreiskWhEVsmall, PreiskWhEVbig, MonatlGrundpreis;




/*
 *
 * TODO: write documentation
 */
void _delay_ms(uint32_t value);
void lcd_print_info(int32_t *Power, int32_t *consumption);
void lcd_refresh_push();
void lcd_refresh_rotary();
void lcd_print_on_off(int pos_line, int pos_row, unsigned flag);
void lcd_print_items();
void lcd_print_checkbox(int pos_line, int pos_row, unsigned flag);
void lcd_clear();
void lcd_print_value_unit(int pos_line, int pos_row, char *value, char* unit);

/**
 \brief Write data to i2c (for internal use)
 \param value byte to send over i2c
 \return none
 */
void lcd_write_i2c(uint8_t value);
int8_t lcd_poll();

/**
 \brief Write nibble to display with toggle of enable-bit
 \param value the upper nibble represents  RS, RW pins and the lower nibble contains data
 \return none
 */
void lcd_write(uint8_t value);

/**
 \brief Read data from i2c (for internal use)
 \retval "uint8_t" byte received over i2c
 */
uint8_t lcd_read_i2c(void);

/**
 \brief Read data from display over i2c (for internal use)
 \param mode ADDRESS for cursor address and busy flag or DATA for display data
 \retval "uint8_t" lower nibble contains data D0 to D3 pins or D4 to D7 pins
 */
uint8_t lcd_read(uint8_t mode);

/**
 \brief Read one byte over i2c from display
 \param mode ADDRESS for cursor address and busy flag or DATA for display data
 \retval "uint8_t" the byte received from the display
 */
uint8_t lcd_getbyte(uint8_t mode);

//-------------------------------------------------------------------------------------------------------------------
/*@}*/

//-FUNCTIONS---------------------------------------------------------------------------------------------------------
/** \defgroup FUNCTIONS FUNCTIONS */
/*@{*/

/**
 \brief Display initialization sequence
 \return none
 */
void lcd_init(void);

/**
 \brief Issue a command to the display
 \param command use the defined commands above
 \return none
 */
void lcd_command(uint8_t command);

/**
 \brief Go to position
 \param line 1st line is 1 and last line = LCD_LINES
 \param col 1st col is 1 and last col = LCD_colS
 \retval true if successfull
 \retval false if not successfull
 */
uint8_t lcd_setcursor(uint8_t line, uint8_t col);

/**
 \brief Put char to cursor position
 \param value the char to print
 \return none
 */
void lcd_putchar(char value);

/**
 \brief Put char to position
 \param line the line to put the char to
 \param col the column to put the char to
 \param value the char to print
 \retval true if successfull
 \retval false if not successfull
 */
uint8_t lcd_putcharlr(uint8_t line, uint8_t col, char value);

/**
 \brief Print string to cursor position
 \param *string pointer to the string to print
 \return none
 */
void lcd_print(char *string);

/**
 \brief Print string from Flash to cursor position
 \param *string pointer to the string to print
 \return none
 */
//void lcd_print_P(PGM_P string);
/**
 \brief Print string to position (If string is longer than LCD_COLS overwrite first chars in line)
 \param line the line to put the string to
 \param col the column to put the string to
 \param *string pointer to the string to print
 \retval true if successfull
 \retval false if not successfull
 */
uint8_t lcd_printlc(uint8_t line, uint8_t col, char *string);

/**
 \brief Print string from Flash to position (If string is longer than LCD_COLS overwrite first chars in line)
 \param line the line to put the string to
 \param col the column to put the string to
 \param *string pointer to the string to print
 \retval true if successfull
 \retval false if not successfull
 */
uint8_t lcd_printlc_P(uint8_t line, uint8_t col, char *string);

/**
 \brief Print string to position (If string is longer than LCD_COLS continue in next line)
 \param line the line to put the string to
 \param col the col to put the string to
 \param *string pointer to the string to print
 \retval true if successfull
 \retval false if not successfull
 */
uint8_t lcd_printlcc(uint8_t line, uint8_t col, char *string);

/**
 \brief Print string from flash to position (If string is longer than LCD_COLS continue in next line)
 \param line the line to put the string to
 \param col the col to put the string to
 \param *string pointer to the string to print
 \retval true if successfull
 \retval false if not successfull
 */
uint8_t lcd_printlcc_P(uint8_t line, uint8_t col, char *string);

/**
 \brief Go to nextline (if next line > LCD_LINES return false)
 \retval true if successfull
 \retval false if not successfull
 */
uint8_t lcd_nextline(void);

/**
 \brief Get line and col of the cursor position
 \param *line pointer to the target byte for line
 \param *col pointer to the target byte for column
 \retval true if successfull
 \retval false if not successfull
 */
uint8_t lcd_getlc(uint8_t *line, uint8_t*col);

/**
 \brief Check if LCD is busy
 \retval true if busy
 \retval false if not busy
 */
uint8_t lcd_busy(void);

/**
 \brief Turn backlight ON/OFF
 \param light true to tun light ON
 \param light false to turn light OFF
 \return none
 */
void lcd_light(uint8_t light);

void lcd_printint(int32_t data);
/*
 *
 * Print arrow on the given position. Delete the old arrow
 *
 */
void lcd_printarrow(int line);

//-------------------------------------------------------------------------------------------------------------------
/*@}*/

#endif
