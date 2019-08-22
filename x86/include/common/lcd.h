
/* $Id: lcd.h,v 1.1.1.1 2003/06/10 22:42:02 iceblink Exp $ */

/*
 * lcd.h
 */
#ifndef COMMON_LCD_H
#define COMMON_LCD_H

#define LCD_PORT 0x0378
#define LCD_DATA_PORT (LCD_PORT+0)
#define LCD_CONTROL_PORT (LCD_PORT+2)

enum LCD_CMDS 
{
    LCD_CMD_INIT1 = 0x38,
    LCD_CMD_INIT2 = 0x06,
    LCD_CMD_INIT3 = 0x0c,
    LCD_CMD_ON = 0x0f,
    LCD_CMD_OFF = 0x08,
    LCD_CMD_RESET1 = 0x3f,
    LCD_CMD_RESET2 = 0x01,
    LCD_CMD_RESET3 = 0x06,
    LCD_CMD_CLEAR = 0x01,
    LCD_CMD_CUR_LEFT = 0x10,
    LCD_CMD_CUR_RIGHT = 0x14,
    LCD_CMD_CUR_OFF = 0x0c,
    LCD_CMD_CUR_ON = 0x0f,
    LCD_CMD_BLINK_OFF = 0x0e
}; 

#define LCD_SET_POS_MASK 0x80
#define LCD_SET_CHAR_MASK 0x40

/* these are actually the bitwise negation of the real values.
 * this is done so they can be passed as masks better
 */
enum BUTTON_CODES
{
    BUTTON_SELECT = 0x80,
    BUTTON_ENTER = 0x40,
    BUTTON_UP = 0x08,
    BUTTON_DOWN = 0x10,
    BUTTON_LEFT = 0x04,
    BUTTON_RIGHT = 0x20,
    BUTTON_RESET = 0x02
};

#define BUTTON_NONE 0xfe
#define BUTTON_MASK 0xfe

/* returns true if all of buttons (bitwise or of above constans) are 
 * the set (value returned from lcd_button_read()
 */
#define lcd_buttons_in_set( set, buttons )  ((((~(set))&0xfe)&buttons)==buttons)
/* returns true if sample contains a subset of set */
#define lcd_buttons_subset( sample, set )  (((~(sample))&0xfe)&set)
#define lcd_button_eq( a, b )  (((~(a))&0xfe)==(b&0xfe) )

void lcd_init( void );
void lcd_write_inst( unsigned char inst );
void lcd_write_data( unsigned char data );
unsigned char lcd_read_inst( void );
unsigned char lcd_read_data( void );
void lcd_on( void );
void lcd_off( void );
void lcd_reset( void );
void lcd_clear( void );
void lcd_cursor_left( void );
void lcd_cursor_right( void );
void lcd_cursor_off( void );
void lcd_cursor_on( void );
void lcd_blink_off( void );
unsigned char lcd_get_cur_pos( void );
void lcd_set_cur_pos( unsigned char pos );
void lcd_write_char( unsigned char c );
void lcd_write_str( char *str );
void lcd_write_str_at(unsigned char pos, char *str);
void lcd_set_char( int c, unsigned char *data );

unsigned char lcd_button_read( void );

void lcd_set_font( int font_num );
void lcd_get_title( char title1[13], char title2[13] );;
void lcd_write_progress_bar( int percent );
void lcd_set_boot_leds( int state );
void lcd_clean_leds( void );

#define lcd_logo_clear() do {           \
	lcd_set_cur_pos(0x44);          \
	lcd_write_str("            ");  \
} while (0)

#define lcd_logo_write(s1, s2) do {     \
	lcd_write_str_at(0x04, (s1));   \
	lcd_write_str_at(0x44, (s2));   \
} while (0)

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
