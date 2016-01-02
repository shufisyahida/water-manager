#include <asf.h>
#include <FreeRTOS.h>
#include <task.h>
#include <touch_button.h>
#include <adc_sensors.h>
#include <stdio.h>
#include <stdlib.h>

void getWaterUsage(void);
void setMaxWater(int maxWater);
void setRainTankVolume(int vol);
void adc_init(void);
void pwm_init(void);
void closeTap(void);
void openTap(void);
void waterAlertOn(void);
void waterAlertOff(void);
void watering(void);

//USART function
void setUpSerial();
void sendChar(char c);
char receiveChar();
void sendString(char *text);
void receiveString();

#define MY_ADC ADCA
#define MY_ADC_CH ADC_CH0

static char strbuf[201];

char reads[100] = "";

int isTap1Opened = 0;
int isTap2Opened = 0;
int isTap3Opened = 0;
int isTap4Opened = 0;
int isWateringTapOpened = 0;
int waterTemp = 0;
int lightIntensity = 0;
int waterUsage = 0;
int maxWater = 0;
int debit = 0;
int rainTankVolume = 0;

void setUpSerial(){
	//Baud rate selection
	//BSEI = (2000000/(2^0 * 16*9600) -1 = 12.0208.... ~12) ->BSALE =0
	
	USARTC0_BAUDCTRLB = 0;
	USARTC0_BAUDCTRLA = 0x0C;
	
	//Disable interrupts, just for safety
	USARTC0_CTRLA =0;
	USARTC0_CTRLC = USART_CHSIZE_8BIT_gc;
	
	USARTC0_CTRLB= USART_TXEN_bm | USART_RXEN_bm;
	
}
void sendChar(char c){
	while (!(USARTC0_STATUS & USART_DREIF_bm));
	
	USARTC0_DATA = c;
	
}

char receiveChar()
{
	while(!(USARTC0_STATUS & USART_RXCIF_bm));
	return USARTC0_DATA;
}
void sendString(char *text){
	while(*text){
		sendChar(*text++);
	}
}
void receiveString()
{
	
	int i =0;
	while(1){
		char inp = receiveChar();
		if(inp=='\n') break;
		else reads[i++] = inp;
	}
}


static void openCloseTap(void *pvParameters)
{
	 while(1){
		 if(gpio_pin_is_low(GPIO_PUSH_BUTTON_0)){
			if(isTap1Opened==0) {
				openTap();
				isTap1Opened = 1;
			} else {
				closeTap();
				isTap1Opened = 0;
			}
			
			if(isTap2Opened==0) {
				openTap();
				isTap2Opened = 1;
				} else {
				closeTap();
				isTap2Opened = 0;
			}
			
			if(isTap3Opened==0) {
				openTap();
				isTap3Opened = 1;
			} else {
				closeTap();
				isTap3Opened = 0;
			}
			
			if(isTap4Opened==0) {
				openTap();
				isTap4Opened = 1;
			} else {
				closeTap();
				isTap4Opened = 0;
			}
		 }
		 vTaskDelay(10);
	 }
}

int getWaterUsage(void) {
	int i=3;
	int Subtotal=0;
	int total=0;
	bool habis = false;
	while(!habis){		
		Subtotal = nvm_eeprom_read_byte(i);
		if(Subtotal>=255){
			i++;			
		}
		else{
			habis=true;
		}
		total+=Subtotal;
		Subtotal=0;
	}
	return total;
}

void setMaxWater(int max) {
	maxWater = max;
}

void setRainTankVolume(int vol) {
	rainTankVolume = vol;
}

//Mengatur debit air (potensiometer)
void adc_init(void) {
	struct adc_config adc_conf;
	struct adc_channel_config adcch_conf;
	
	adc_read_configuration(&MY_ADC, &adc_conf);
	adcch_read_configuration(&MY_ADC, MY_ADC_CH, &adcch_conf);
	
	adc_set_conversion_parameters(&adc_conf, ADC_SIGN_OFF, ADC_RES_12, ADC_REF_VCC);
	adc_set_conversion_trigger(&adc_conf, ADC_TRIG_MANUAL, 1, 0);
	adc_set_clock_rate(&adc_conf, 200000UL);
	
	adcch_set_input(&adcch_conf, J3_PIN0, ADCCH_NEG_NONE, 1);
	
	adc_write_configuration(&MY_ADC, &adc_conf);
	adcch_write_configuration(&MY_ADC, MY_ADC_CH, &adcch_conf);
}

static void setWaterDebit(void *pvParameters)
{
	while (1) {
		//uint16_t result;
		adc_enable(&MY_ADC);
		adc_start_conversion(&MY_ADC, MY_ADC_CH);
		adc_wait_for_interrupt_flag(&MY_ADC, MY_ADC_CH);
		debit = adc_get_result(&MY_ADC, MY_ADC_CH)/1000;
		vTaskDelay(500);
		
	}
}

//Watering from rain tank
static void watering(void *vpParameters) {
	int isAuto = 0;
	if(isAuto==1) {
		if(lightIntensity>=150  && lightIntensity<=200) {
			int wateringVol = 500;
			isWateringTapOpened = 1;
			openTap();
			LED_On(LED1);
			LED_On(LED2);
			while(wateringVol>=0) {
				wateringVol--;
				vTaskDelay(5);
			}
			rainTankVolume -= wateringVol;
			isWateringTapOpened=0;
			closeTap();
			LED_Off(LED1);
			LED_Off(LED2);
		}
	} else {
		int flag = 0;	//terima dari HG
		if(flag==0) {
			isWateringTapOpened = 1;
			openTap();
			LED_On(LED1);
			LED_On(LED2);
			while(rainTankVolume>=0) {
				rainTankVolume--;
				vTaskDelay(5);
			}
		} else {
			isWateringTapOpened=0;
			closeTap();
			LED_Off(LED1);
			LED_Off(LED2);
		}
	}
} 

//Menghitung penggunaan air (eeprom)
static void countWaterUsage(void *vpParameters) {
	while(1) {
		if(isTapOpened==1) {
			int i=3;
			int Subtotal=0;
			int total=0;
			int index=0;
			bool habis = false;
			while(!habis){
				Subtotal = nvm_eeprom_read_byte(i);
				if(Subtotal>=255){
					i++;
				}
				else{
					habis=true;
				}
				total+=Subtotal;
				Subtotal=0;
				index = i;
			}
			waterUsage = total;
			nvm_eeprom_write_byte(index, total);
		}
		vTaskDelay(5);
	}
}

//Membuka dan menutup keran (servo)
void pwm_init(void){
	PORTC.DIR |= PIN0_bm;
	
	TCC0.CTRLA = (PIN2_bm) | (PIN0_bm);
	TCC0.CTRLB = (PIN4_bm) | (PIN2_bm) | (PIN1_bm);
	
	TCC0.PER = 8000;
	TCC0.CCA = 375;
}

void closeTap(void){
	PORTC.DIR |= PIN0_bm;
	
	TCC0.CTRLA = (PIN2_bm) | (PIN0_bm);
	TCC0.CTRLB = (PIN4_bm) | (PIN2_bm) | (PIN1_bm);
	
	TCC0.PER = 8000;
	TCC0.CCA = 375;
}
void openTap(void){
	PORTC.DIR |= PIN0_bm;
	
	TCC0.CTRLA = (PIN2_bm) | (PIN0_bm);
	TCC0.CTRLB = (PIN4_bm) | (PIN2_bm) | (PIN1_bm);
	
	TCC0.PER = 8000;
	TCC0.CCA = 1;
}

//Memberi peringatan setiap menggunakan air saat penggunaan air sudah mencapai batas maksimal (buzzer)
void waterAlertOn(void) {
	PORTE_DIR=0b00000001;
	PORTE_OUT=0b00000001;
}

void waterAlertOff(void) {
	PORTE_DIR=0b00000001;
	PORTE_OUT=0b00000000;
}

static void waterAlertTask(void *pvParameters) {
	while(1) {
		gpio_set_pin_high(NHD_C12832A1Z_BACKLIGHT);
		if(waterUsage > maxWater && isTapOpened==1) {
			waterAlertOn();
			vTaskDelay(50);
			waterAlertOff();
		}
		vTaskDelay(10);
	}
}

//Mengukur intensitas cahaya
static void checkWater(void *pvParameters)
{
	while(1)
	{
		// Start an ADC conversion of the lightsensor
		lightsensor_measure(); //memulai membaca nilai sensor cahaya dari adc
		if(lightsensor_data_is_ready()) //jika nilainya sensor cahaysa sudah didapat
		{
			lightIntensity = lightsensor_get_raw_value(); //membaca nilai sensor cahaya
		}
		vTaskDelay(10);
	}
	
}
//Menampilkan status penggunaan air, status keran, suhu air (LCD)
void clearLCD(void){
	int page,column;
	for (page = 0; page < GFX_MONO_LCD_PAGES; page++) {
		for (column = 0; column < GFX_MONO_LCD_WIDTH; column++) {
			gfx_mono_put_byte(page, column, 0x00);
		}
	}
}

static void vLCD(void *pvParameters)
{
	gfx_mono_draw_string("Water   : ", 0, 16, &sysfont);
	gfx_mono_draw_string("Debit   : ", 0, 24, &sysfont);
	gfx_mono_draw_string("Keran   : ", 0, 8, &sysfont);
	gfx_mono_draw_string("Usage   : ", 0, 0, &sysfont);
	gpio_set_pin_high(NHD_C12832A1Z_BACKLIGHT);
	while(1){
		if(isTapOpened==1) gfx_mono_draw_string("Terbuka ", 60, 8, &sysfont);
		if(isTapOpened==0) gfx_mono_draw_string("Tertutup", 60, 8, &sysfont);
		
		snprintf(strbuf, sizeof(strbuf), "%3d", waterTemp);
		gfx_mono_draw_string(strbuf, 60, 16, &sysfont);
		
		snprintf(strbuf, sizeof(strbuf), "%3d", debit);
		gfx_mono_draw_string(strbuf, 60, 24, &sysfont);
		
		snprintf(strbuf, sizeof(strbuf), "%3d", waterUsage);
		gfx_mono_draw_string(strbuf, 60, 0, &sysfont);
		
		snprintf(strbuf, sizeof(strbuf), "%3d", waterIntensity);
		gfx_mono_draw_string(strbuf, 60, 16, &sysfont);
		
		vTaskDelay(1);
		//clearLCD();
	}
}

//tanda kerannya terbuka atau tertutup (led)
static void checkTap(void *pvParameters) {
	while(1) {
		if(isTap1Opened == 0) {
			LED_Off(LED0);
		} else {
			LED_On(LED0);
		}
		
		if(isTap2Opened == 0) {
			LED_Off(LED1);
		} else {
			LED_On(LED1);
		}
		
		if(isTap3Opened == 0) {
			LED_Off(LED2);
		} else {
			LED_On(LED2);
		}
		
		if(isTap4Opened == 0) {
			LED_Off(LED3);
			} else {
			LED_On(LED3);
		}
		vTaskDelay(1);
	}
}

int main (void)
{
	board_init(); //konfigurasi awal board
	sysclk_init(); //konfigurasi awal system clock
	adc_sensors_init();// konfigurasi adc
	gfx_mono_init(); //konfigurasi awal LCD monochrom
	tb_init(); //konfigurasi touch button
	cpu_irq_enable(); // konfigurasi untuk menghidupkan interrupt
	pmic_init(); //konfigurasi untuk menyalakan semua interrupt dan mengatur prioritas task
	pwm_init();
	adc_init();
	
	PORTC_OUTSET = PIN3_bm;
	PORTC_DIRSET = PIN3_bm;
	
	PORTC_OUTCLR = PIN2_bm;
	PORTC_DIRCLR = PIN2_bm;
	
	setUpSerial();
	
	waterUsage = nvm_eeprom_read_byte(1);
	setMaxWater(200);
	xTaskCreate(openCloseTap, "", 200, NULL, 1, NULL);
	xTaskCreate(setWaterDebit, "", 200, NULL, 1, NULL);
	xTaskCreate(countWaterUsage, "", 400, NULL, 1, NULL);
	xTaskCreate(waterAlertTask, "", 400, NULL, 1, NULL);
	xTaskCreate(checkWater, "", 200, NULL, 1, NULL);
	xTaskCreate(vLCD, "", 600, NULL, 1, NULL);
	xTaskCreate(checkTap, "", 200, NULL, 1, NULL);

	vTaskStartScheduler();
}