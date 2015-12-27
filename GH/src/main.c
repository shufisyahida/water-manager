#include <asf.h>
#include <FreeRTOS.h>
#include <task.h>
#include <touch_button.h>
#include <adc_sensors.h>
#include <stdio.h>
#include <sysclk.h>
						

static portTASK_FUNCTION_PROTO(printLCD, p_);
static portTASK_FUNCTION_PROTO(button, p_);
static portTASK_FUNCTION_PROTO(menuNav, p_);
static portTASK_FUNCTION_PROTO(Touch, p_);
static portTASK_FUNCTION_PROTO(commGate_IN, p_);
static portTASK_FUNCTION_PROTO(commGate_OUT, p_);
static portTASK_FUNCTION_PROTO(commGate_SIG, p_);

static volatile bool flagT;
static volatile int flagB;

static char strbuf[201];
static char reads[100];

static char strbuf_menu[20];
static char strbuf_in[20];
static char strbuf_send[50];
static char strbuf_read[50];
static char *featureList[15];
static bool featureStat[15];
static char *statusList[6];

char up[20] = "coba panjang atas \n";
char down[20] = "coba panjang bawah \n";
char still[20] = "coba panjang diam \n";

bool is_sending = false;
bool is_receive = false;

int menuSelected = 1;
int maxFeature = 14;
int maxStatus = 6;
bool switchDisp = false;
int feature_selected = 0;
int status_displayed = 0;

static usart_rs232_options_t USART_SERIAL_OPTIONS = {
	.baudrate = USART_SERIAL_EXAMPLE_BAUDRATE,
	.charlength = USART_SERIAL_CHAR_LENGTH,
	.paritytype = USART_SERIAL_PARITY,
	.stopbits = USART_SERIAL_STOP_BIT
};

void setUpSerial()
{
	// Baud rate selection
	// BSEL = (2000000 / (2^0 * 16*9600) -1 = 12.0208... ~ 12 -> BSCALE = 0
	// FBAUD = ( (2000000)/(2^0*16(12+1)) = 9615.384 -> mendekati lah ya
	
	USARTC0_BAUDCTRLB = 0; //memastikan BSCALE = 0
	USARTC0_BAUDCTRLA = 0x0C; // 12
	//USARTC0_BAUDCTRLB = 0; //Just to be sure that BSCALE is 0
	//USARTC0_BAUDCTRLA = 0xCF; // 207
	
	
	//Disable interrupts, just for safety
	USARTC0_CTRLA = 0;
	//8 data bits, no parity and 1 stop bit
	USARTC0_CTRLC = USART_CHSIZE_8BIT_gc;
	
	//Enable receive and transmit
	USARTC0_CTRLB = USART_TXEN_bm | USART_RXEN_bm;
}

void sendChar(char c)
{
	portENTER_CRITICAL();
	while( !(USARTC0_STATUS & USART_DREIF_bm) ); //Wait until DATA buffer is empty
	portEXIT_CRITICAL();
	USARTC0_DATA = c;
	
}

void sendString(char *text)
{
	is_sending=true;
	//usart_putchar(USART_SERIAL_EXAMPLE, 0b10101011);
	while(*text)
	{
		//sendChar(*text++);
		//portENTER_CRITICAL();
		usart_putchar(USART_SERIAL_EXAMPLE, *text++);
		//portEXIT_CRITICAL();
	}
	//usart_putchar(USART_SERIAL_EXAMPLE, '\n');
	is_sending=false;
}

char receiveChar()
{
	portENTER_CRITICAL();
	while( !(USARTC0_STATUS & USART_RXCIF_bm) ); //Wait until receive finish
	portEXIT_CRITICAL();
	return USARTC0_DATA;
}

void receiveString()
{
	int i = 0;
	while(1){
		//char inp = receiveChar();
		//portENTER_CRITICAL();
		char inp = usart_getchar(USART_SERIAL_EXAMPLE);
		if(inp=='\n') break;
		else strbuf_read[i++] = inp;
		//portEXIT_CRITICAL();
	}/*
	if(usart_getchar(USART_SERIAL_EXAMPLE)==0b10101011) {
		while(1){
			//char inp = receiveChar();
			portENTER_CRITICAL();
			char inp = usart_getchar(USART_SERIAL_EXAMPLE);
			if(inp=='\n') break;
			else strbuf_read[i++] = inp;
			portEXIT_CRITICAL();
			int a = 0;
			int b = 1;
			int c = 2;
			int d = 3;
		}
	}*/
}

void switchDisplay(int dest){
	menuSelected = dest;
	switchDisp = true;
}

void initMenu(void){
	
	int nn;
	for(nn=0; nn<=maxFeature; nn++){
		//inisialisasi fitur, awalnya empty
		snprintf(strbuf_in, sizeof(strbuf_in),"empty", nn);
		featureList[nn] = strbuf_in;
		featureStat[nn] = false;
	}
	for(nn=0; nn<=maxStatus; nn++){
		statusList[nn]="...";
	}
}

/************************************************************************/
/* Main                                                                     */
/************************************************************************/
int main (void)
{
	sysclk_init();
	board_init();

	gfx_mono_init();
	tb_init();
	//adc_sensors_init();
	
	usart_init_rs232(USART_SERIAL_EXAMPLE, &USART_SERIAL_OPTIONS);
	//usart_set_mode(USART_SERIAL_EXAMPLE,USART_CMODE_SYNCHRONOUS_gc);
	
	PORTC_OUTCLR = PIN2_bm; //PC2 as RX
	PORTC_DIRCLR = PIN2_bm; //RX pin as input
	
	PORTC_OUTSET = PIN3_bm; //PC3 as TX
	PORTC_DIRSET = PIN3_bm; //TX pin as output
	
	PORTC_OUTCLR = PIN4_bm; //PC4 as sig-in
	PORTC_DIRCLR = PIN4_bm; //PC4 pin as input
	
	PORTC_OUTSET = PIN3_bm; //PC5 as sig-out
	PORTC_DIRSET = PIN3_bm; //PC5 pin as output

	//setUpSerial();
	
	xTaskCreate(printLCD,"",500,NULL,1,NULL);
	xTaskCreate(button,"",500,NULL,1,NULL);
	xTaskCreate(menuNav,"",500,NULL,1,NULL);
	xTaskCreate(Touch, "", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate(commGate_IN, "",500, NULL, 0, NULL);
	xTaskCreate(commGate_OUT, "", 500, NULL, 0, NULL);
	xTaskCreate(commGate_SIG, "", 500, NULL, 0, NULL);

	
	vTaskStartScheduler();
}

static portTASK_FUNCTION(printLCD, p_){
	ioport_set_pin_level(LCD_BACKLIGHT_ENABLE_PIN, 1);
	initMenu();
	while(1){
		
		if(switchDisp){
			portENTER_CRITICAL();
			gfx_mono_generic_draw_filled_rect(0,0,128,32,GFX_PIXEL_CLR);
			vTaskDelay(200/portTICK_PERIOD_MS);
			portEXIT_CRITICAL();
			switchDisp = false;
		}
		
		//main menu
		if(menuSelected==0){
			
			//snprintf(strbuf_menu, sizeof(strbuf_menu), "Menu %d", menuSelected);
			//gfx_mono_draw_string(strbuf_menu,0, 0, &sysfont);
			gfx_mono_draw_string("==GH Control Center==",0,0,&sysfont);
			gfx_mono_draw_string("---------------------",0,(SYSFONT_HEIGHT*1)+1,&sysfont);
			gfx_mono_draw_string("Ping         Features",0,(SYSFONT_HEIGHT*2)+1,&sysfont);
			gfx_mono_draw_string("PingBrst    StatsDisp",0,(SYSFONT_HEIGHT*3)+2,&sysfont);	
		}
		//ping
		else if(menuSelected==1){
			gfx_mono_draw_string("==    Ping Menu    ==",0,0,&sysfont);
			//if(still==still) gfx_mono_draw_string("still",0,(SYSFONT_HEIGHT*1)+1,&sysfont);
			gfx_mono_draw_string(strbuf_read,0,(SYSFONT_HEIGHT*1)+1,&sysfont);
			gfx_mono_draw_string(strbuf_send,0,(SYSFONT_HEIGHT*2)+1,&sysfont);
		}
		//ping burst
		else if(menuSelected==2){
			gfx_mono_draw_string("==  PingBrst Menu  ==",0,0,&sysfont);
		}
		//features
		else if(menuSelected==3){
			
			if(feature_selected<0) feature_selected=maxFeature;
			else if (feature_selected>maxFeature) feature_selected=0;
			
			gfx_mono_draw_string("==  Features Menu  ==",0,0,&sysfont);
			
			snprintf(strbuf_menu, sizeof(strbuf_menu), "Feature No.%2d", feature_selected+1);
			gfx_mono_draw_string(strbuf_menu,0, (SYSFONT_HEIGHT)+1, &sysfont);
			gfx_mono_draw_string(featureList[feature_selected],0, (SYSFONT_HEIGHT*2)+1, &sysfont);
			gfx_mono_draw_string("   A             B   ",0, (SYSFONT_HEIGHT*3)+3, &sysfont);
			if(!featureStat[feature_selected]){
				gfx_mono_generic_draw_filled_rect((SYSFONT_WIDTH*2)-1,(SYSFONT_HEIGHT*3)+2,SYSFONT_WIDTH*3,SYSFONT_HEIGHT+2,GFX_PIXEL_XOR);
			}else{
				gfx_mono_generic_draw_filled_rect((SYSFONT_WIDTH*16)-1,(SYSFONT_HEIGHT*3)+2,SYSFONT_WIDTH*3,SYSFONT_HEIGHT+2,GFX_PIXEL_XOR);
			}
		}
		//status display
		else if(menuSelected==4){
			gfx_mono_draw_string("==   SD's Status   ==",0,0,&sysfont);
			int jj;
			for(jj=0; jj<maxStatus; jj++){
				
			}
			gfx_mono_draw_string(statusList[status_displayed],0, (SYSFONT_HEIGHT)+1, &sysfont);
		}
			
		//gfx_mono_draw_string("<",SYSFONT_WIDTH*19, selected_menu*10, &sysfont);
			
		/*
		int ii;
		for(ii=0; ii<sizeof(displayed_menu); ii++){
			int num = displayed_menu[ii];
			//snprintf(strbuf_menu, sizeof(strbuf_menu), menuList[num]);
			gfx_mono_draw_string(menuList[num],0, num*10, &sysfont);
			//vTaskDelay(1/portTICK_PERIOD_MS);
		}*/
			
		//snprintf(strbuf_menu, sizeof(strbuf_menu), "<<");
		//gfx_mono_draw_string("<",SYSFONT_WIDTH*19, selected_menu*10, &sysfont);
		//gfx_mono_generic_draw_filled_rect(111,0,8,32,GFX_PIXEL_CLR);
			

		//snprintf(strbuf_menu, sizeof(strbuf_menu), "%d",selected_menu);
		//gfx_mono_draw_string(strbuf_menu,0, 0, &sysfont);
		//vTaskDelay(10/portTICK_PERIOD_MS);
		//gfx_mono_generic_draw_filled_rect(0,selected_menu*10,128,32,GFX_PIXEL_XOR);
		//vTaskDelay(1000/portTICK_PERIOD_MS);
		vTaskDelay(50/portTICK_PERIOD_MS);
		//gfx_mono_generic_draw_filled_rect(111,0,8,32,GFX_PIXEL_CLR);

	}
}

static portTASK_FUNCTION(button, p_){
	while(1){
		vTaskDelay(100/portTICK_PERIOD_MS);
		
		if(gpio_pin_is_low(GPIO_PUSH_BUTTON_0)){
			gpio_set_pin_low(LED0_GPIO);
			flagB = 0;
		}else if(gpio_pin_is_low(GPIO_PUSH_BUTTON_1)){
			gpio_set_pin_low(LED2_GPIO);
			flagB = 1;
		}else if(gpio_pin_is_low(GPIO_PUSH_BUTTON_2)){
			gpio_set_pin_low(LED3_GPIO);
			flagB = 2;
		}else if(gpio_pin_is_high(GPIO_PUSH_BUTTON_1) && gpio_pin_is_high(GPIO_PUSH_BUTTON_2) && gpio_pin_is_high(GPIO_PUSH_BUTTON_0)){
			gpio_set_pin_high(LED3_GPIO);
			gpio_set_pin_high(LED2_GPIO);
			gpio_set_pin_high(LED0_GPIO);
			flagB = 3;
		}
		
		//snprintf(strbuf_in, sizeof(strbuf_in), "%d",selected_menu);
		//gfx_mono_draw_string(strbuf_in,0, 0, &sysfont);
		
		//gfx_mono_draw_string("<",SYSFONT_WIDTH*19, selected_menu*10, &sysfont);
	}
}

/************************************************************************/
/* Fungsi Trigger Action Select                                         */
/************************************************************************/
static portTASK_FUNCTION(Touch, p_) {
	while(1) {
		flagT=!tb_is_touched();
		vTaskDelay(100 / portTICK_PERIOD_MS);				
		
		if(flagT){
			gpio_set_pin_low(LED1_GPIO);
		}else{
			gpio_set_pin_high(LED1_GPIO);
		}
	}
}

static portTASK_FUNCTION(menuNav, p_){
	while(1){
		
		vTaskDelay(100 / portTICK_PERIOD_MS);
		
		//main menu
		if(menuSelected==0){
			if(flagB==0) switchDisplay(1);		//goto ping
			else if(flagB==1) switchDisplay(3);	//goto ping burst
			else if(flagB==2) switchDisplay(4);	//goto features
			if (flagT) switchDisplay(2);		//goto ping burst 
		}
		//ping
		else if(menuSelected==1){
			if(flagB==0) switchDisplay(0);		//back - goto main
			else if(flagB==1) {snprintf(strbuf_send, sizeof(strbuf_send), "coba panjang atas \n");}
			else if(flagB==2) {snprintf(strbuf_send, sizeof(strbuf_send), "coba panjang bawah \n");}
			else if(flagB==3) {snprintf(strbuf_send, sizeof(strbuf_send), "coba panjang diam \n");}
			if (flagT) {						//execute!
				//ping started
			}
			
			
			
		}
		//ping burst
		else if(menuSelected==2){
			if(flagB==0) switchDisplay(0);		//back - goto main
			else if(flagB==1) {}
			else if(flagB==2) {}
			if (flagT) {						//execute!
				//ping started
			}
		}
		//features
		else if(menuSelected==3){
			bool statSw = featureStat[feature_selected];
			if(flagB==0) switchDisplay(0);		//back - goto main
			else if(flagB==1) feature_selected--; //go up
			else if(flagB==2) feature_selected++; //go down
			if (flagT) {						//execute!
				//switch stat
				if(statSw) featureStat[feature_selected]=false;
				else featureStat[feature_selected]=true; 
			}
			
		}
		//status display
		else if(menuSelected==4){
			if(flagB==0) switchDisplay(0);		//back - goto main
			else if(flagB==1) {}				//scroll up
			else if(flagB==2) {}				//scroll down
			if (flagT) {						//execute!
				//ping started
				
			}
		}

	}
}
	

static portTASK_FUNCTION(commGate_IN, p_){
	while(1){
		vTaskDelay(1 / portTICK_PERIOD_MS);
		//portENTER_CRITICAL();
		/*
		if(is_receive==true) {
			receiveString();
		}*/
		receiveString();
		//portEXIT_CRITICAL();
	}
}

static portTASK_FUNCTION(commGate_OUT, p_){
	while(1){
		vTaskDelay(1 / portTICK_PERIOD_MS);
		//portENTER_CRITICAL();
		sendString(strbuf_send);
		//portEXIT_CRITICAL();
	}
}

static portTASK_FUNCTION(commGate_SIG, p_){
	while(1){
		//transmit status
		ioport_set_pin_level(PORTC_PIN5CTRL,is_sending);
		
		//receive status
		is_receive = ioport_get_pin_level(PORTC_PIN4CTRL);
		//is_receive = true;
		
		ioport_set_pin_level(LED0_GPIO,is_sending);
		ioport_set_pin_level(LED1_GPIO,is_receive);
	}
}