/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# "Insert system clock initialization code here" comment
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
#include <asf.h>
#include <usart.h>
#include <usart_serial.h>

void setUpSerial(void);
void sendChar(char c);
void receiveChar(void);

static char strbuf[201];
char in;

void setUpSerial(void) {
	USARTC0_BAUDCTRLB = 0;
	USARTC0_BAUDCTRLA = 0x0C;
	
	USARTC0_CTRLA = 0;
	USARTC0_CTRLC = USART_CHSIZE_8BIT_gc;
	
	USARTC0_CTRLB = USART_TXEN_bm | USART_RXEN_bm;
}

void sendChar(char c) {
	while(!(USARTC0_STATUS & USART_DREIF_bm));
	USARTC0_DATA = c;
}

char receiveChar() {
	while(!(USARTC0_STATUS & USART_RXCIF_bm));
	return USARTC0_DATA;
}

int main (void)
{
	// Insert system clock initialization code here (sysclk_init()).

	board_init();
	sysclk_init();
	gfx_mono_init();

	// Insert application code here, after the board has been initialized.
	
	gpio_set_pin_high(LCD_BACKLIGHT_ENABLE_PIN);
	
	PORTC_OUTSET = PIN3_bm;
	PORTC_DIRSET = PIN3_bm;
	
	PORTC_OUTCLR = PIN2_bm;
	PORTC_DIRCLR = PIN2_bm;
	
	setUpSerial();
	
	while(1) {
		if(gpio_pin_is_low(GPIO_PUSH_BUTTON_1) && gpio_pin_is_high(GPIO_PUSH_BUTTON_2)) {
			gpio_set_pin_low(LED2_GPIO);
			gpio_set_pin_high(LED3_GPIO);
			sendChar('q');
		} else if(gpio_pin_is_low(GPIO_PUSH_BUTTON_2) && gpio_pin_is_high(GPIO_PUSH_BUTTON_1)) {
			gpio_set_pin_low(LED3_GPIO);
			gpio_set_pin_high(LED2_GPIO);
			sendChar('a');
		} else if(gpio_pin_is_high(GPIO_PUSH_BUTTON_1) && gpio_pin_is_high(GPIO_PUSH_BUTTON_2)) {
			gpio_set_pin_high(LED3_GPIO);
			gpio_set_pin_high(LED2_GPIO);
			sendChar('g');
		}
		
		in = receiveChar();
		
		snprintf(strbuf, sizeof(strbuf), "Read USART: %3d", in);
		gfx_mono_draw_string(strbuf, 0, 0, &sysfont);
		delay_ms(50);
	}
}
