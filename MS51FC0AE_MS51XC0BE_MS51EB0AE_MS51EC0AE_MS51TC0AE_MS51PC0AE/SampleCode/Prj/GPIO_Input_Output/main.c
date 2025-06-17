#include "ms51_32k.h"
#include "string.h"
bit  ADC_CONT_FINAL_FLAG; 
//define
#define delay_ms(x) Timer0_Delay(24000000,x,1000) 
#define delay_us(x) Timer0_Delay(24000000,x,100) 

#define LED_GREEN P04
#define LED_RED P05
#define BEEP P12
#define EN_WIFI P34

#define KEYB P01
#define KEYC P33
#define RF_IN P03

#define DC_ON P14
#define CH_OFF P22
#define BOOST_OFF P32

#define RL1 P37
#define RL2 P36
#define RL3 P35
#define RL4 P31

#define BEEP_OFF BEEP = 0;
#define BEEP_ON BEEP = 1;

#define BELL_ON RL2 = 1;
#define BELL_OFF RL2 = 0;

#define PW_ACIN         0x01
#define PW_BAT          0x02
#define PW_CHARG        0x04
#define BAT_OK          0x08

#define ALARM           0x00
#define NOT_USE         0x01 
#define ENDOFLINE       0x02 
#define NORMAL          0x03

#define LINE16_K1           0x00
#define LINE16_K2         0x01 

char dataload[40];
char HexString[73];
int Tick = 0;
//server
char *test_mqttServer = "\"dev-api.lctech.vn\"";
int   test_mqttPort = 1884;
char *mqttUser = "\"lctech_dev\"";
char *mqttPassword = "\"lctech_DEV@123\"";
char *topicSever = "\"swm/g4/data\"";
char *txt_subscribe = "\"mmm/94E6862F1EA0/cmd\"";
//char ESP_ID[15]; 
char ESPID[15] ;
char IMEI[20]; 
char M[100];
//uart interrupt
unsigned char RxUART[100];
unsigned int ID = 0;
int update = 0; 

#define numLine 16;
struct gsafe_msg_data { 
  uint8_t alarmStatus;        //  1        
  uint16_t batVoltage;        //  2 
  unsigned int batPercent;         //  1 
  unsigned int pwStatus;           //  1 
  int Status;             //  1 
  unsigned int rssi4G;              //  1 
  unsigned int rssiWF;              //  1 
	unsigned int rssiInit;
  unsigned int lineStatus[16];
  int holdStatus[16];
	int keySensor[16],timeHold[16];
  int enableBell;   
};
struct gsafe_msg_data gdata ;
//fucntion
int Channel_Alarm[16];
void Set_Channel_ADC(char kenh){
	//p16 p02 p15 p25
	P25 = (kenh)&0x01;
	P15 = (kenh>>1)&0x01;
	P02 = (kenh>>2)&0x01;
	P16 = (kenh>>3)&0x01;
}
void Set_Relay(int status){
	P37 = status;  			//RL1
	P36 = status;
	P35 = status;
	P31 = status;
}
unsigned int readADC_Sensor(char channel){
	unsigned int ADCdataAIN;
	Set_Channel_ADC(channel);
	ENABLE_ADC_CH0;
	clr_ADCCON0_ADCF;
	set_ADCCON0_ADCS; // ADC start trig signal
	while(ADCF == 0);
	P35 ^= 1;
	ADCdataAIN = (ADCRH<<4)+(ADCRL&0x0F);
	DISABLE_ADC;
	return ADCdataAIN;
}
unsigned int adc[16];
void readADC_16line(){
	int line;
	gdata.Status = NORMAL;
	for(line=0;line<16;line++){
		adc[line] = readADC_Sensor(line);
		//k mac tro: adc = 3400-3600
		//mac tro binh thuong: 3000
		//mac tro bao chay: <1000
		if(adc[line] >3200){//not use
			Channel_Alarm[line] = 1;
			gdata.holdStatus[line] = NOT_USE;
			gdata.lineStatus[line]=NOT_USE;
		}
		else if (adc[line] <1000){//alarm
			Channel_Alarm[line] = 2;
			gdata.lineStatus[line]=ALARM;
			if(gdata.keySensor[line]!=ALARM){ 
          gdata.keySensor[line] = ALARM;
          gdata.timeHold[line] = 1;
      } else{
				gdata.timeHold[line]++;
				if(gdata.timeHold[line]>=5){
					gdata.timeHold[line]=5;
					gdata.holdStatus[line] = ALARM;
					gdata.Status = ALARM;  
				}
				}	
		}
		else {//normal
			Channel_Alarm[line] = 3;
			gdata.timeHold[line]=0;
			gdata.holdStatus[line] = NORMAL; 
			gdata.keySensor[line] = NORMAL;  
			gdata.lineStatus[line]=NORMAL;
		}
	}
}


void putc_UART0(unsigned int *mang, unsigned int n){
	unsigned int i;
	for(i=0;i<n;i++){
		UART_Send_Data(UART0,mang[i]);
	}
}
void putc_UART1(unsigned int *mang, unsigned int n){
	unsigned int i;
	for(i=0;i<n;i++){
		UART_Send_Data(UART1,mang[i]);
	}
}
void putc_UART2(unsigned int *mang, unsigned int n){
	unsigned int i;
	for(i=0;i<n;i++){
		UART2_Send_Data(mang[i]);
	}
}
void puts_UART0(char *chuoi){
	while(*chuoi){
		UART_Send_Data(UART0,*chuoi);
		chuoi++;
	}
}
void puts_UART1(char *chuoi){
	while(*chuoi){
		UART_Send_Data(UART1,*chuoi);
		chuoi++;
	}
}
void puts_UART2(char *chuoi){
	while(*chuoi){
		UART2_Send_Data(*chuoi);
		chuoi++;
	}
}

void checkAlarm(){
	static int keyBell = 0;  
	static int stt = 1;     
	static int alarm = 0;

	if(gdata.Status == ALARM){
		if(alarm != ALARM){
			BELL_ON;
			BEEP_ON;
			keyBell = 1;
		}
		if(KEYB == 0 && stt == 1){
			if(keyBell){
				BELL_OFF;
				BEEP_OFF;
				keyBell = 0;
			} else {
				BELL_ON;
				BEEP_ON;
				keyBell = 1;
			}
		}
		RL1 = 1; RL3 = 1; 
		RL4= 1;
	}
	else {
		BELL_OFF;
		BEEP_OFF;
		//Set_Relay(0);
		RL1 = 0;
		RL2 = 0; 
//		RL3 = 0;
		RL4 = 0;
		keyBell = 0;
	}
	stt = KEYB;
	alarm = gdata.Status;
}


			
unsigned int check_power(){
	unsigned int ac_ch;
	ENABLE_ADC_CH12;
	clr_ADCCON0_ADCF;
	set_ADCCON0_ADCS;
	while(ADCF == 0);
	P35 ^= 1;
	ac_ch = (ADCRH<<4)+(ADCRL&0x0F);
	DISABLE_ADC;
	return ac_ch;
}
unsigned int check_Vdc(){
	unsigned int value;
	ENABLE_ADC_CH13;
	clr_ADCCON0_ADCF;
	set_ADCCON0_ADCS; 
	while(ADCF == 0);
	P35 ^= 1;
	value = (ADCRH<<4)+(ADCRL&0x0F);
	DISABLE_ADC;
	return value;
}
unsigned int check_Vbat(){
	unsigned int value;
	ENABLE_ADC_CH11;
	clr_ADCCON0_ADCF;
	set_ADCCON0_ADCS; 
	while(ADCF == 0);
	P35 ^= 1;
	value = (ADCRH<<4)+(ADCRL&0x0F);
	DISABLE_ADC;
	return value;
}
unsigned int check_IN16M(){
	unsigned int value;
	ENABLE_ADC_CH7;
	clr_ADCCON0_ADCF;
	set_ADCCON0_ADCS; 
	while(ADCF == 0);
	P35 ^= 1;
	value = (ADCRH<<4)+(ADCRL&0x0F);
	DISABLE_ADC;
	//1.65 
	if(value>1600){
		return LINE16_K2;
	}
	else{
		return LINE16_K1;
	}
}
void controlPower(){
				//control vdc,vbat
		unsigned int vbat_fb,vdc_fb,pw_fb;
		vbat_fb = check_Vbat(); // control 
		vdc_fb = check_Vdc();// control DC_ON
		pw_fb = check_power();
		gdata.pwStatus = 0;
		gdata.batPercent = (int)((vbat_fb-2700)/8);
		if(gdata.batPercent>100)gdata.batPercent=100;
		
		if(vdc_fb<2500){ //Vdc<23V
			BOOST_OFF = 0;
		} else {
			BOOST_OFF = 1;
		}
		if(vbat_fb<3500){//<8.4 open charge
				CH_OFF = 0; 
		}
		if( pw_fb>3700){
			gdata.pwStatus |= PW_BAT;	
//			return 1;// khoong co AC
		} else if (pw_fb < 1600){
			gdata.pwStatus |= PW_ACIN;
			gdata.pwStatus |= BAT_OK;
//			return 2; //co nguon AC, Pin
		} else {
			gdata.pwStatus |= PW_ACIN;
			gdata.pwStatus |= PW_CHARG;
//			return 3; // co AC, k pin
		}
}


unsigned char bufffer[100];
//void Read_String0(char *buffer,unsigned char len){
//	int i;
//	unsigned char dt;
//	for(i=0;i<len;i++){
//		dt = Receive_Data(UART0);
//		*buffer = dt;
//		buffer++;	
//	}
//	*buffer = '\0';
//}
//void Read_String1(char *buffer,unsigned char len){
//	int i;
//	unsigned char dt;
//	for(i=0;i<len;i++){
//		dt = Receive_Data(UART1);
//		*buffer = dt;
//		buffer++;	
//	}
//	*buffer = '\0';
//}
//void ReadStringUntil0(unsigned char *buffer,unsigned char until, int max_length){
//	int i;
//	unsigned char dt;
//	char *ptr = buffer;
//	for(i=0;i<max_length;i++){
//		dt = Receive_Data(UART0);
//		if(dt == until) break;
//		*ptr = dt;
//		ptr+=1;
//	}
//	*ptr = '\0';
//}
//unsigned char Receive_Data0(unsigned int timeout)
//{
//    UINT8 c;
//		//unsigned char c;
//		PUSH_SFRS;
//    SFRS = 0;
//		while (!RI){
//			if(Tick > timeout) return 0;
//		}
//		c = SBUF;
//		RI = 0;
//		POP_SFRS;
//    return (c);
//}
//void ReadStringUntil00(unsigned char *buffer,unsigned char until, unsigned int timeout){
//	int i;
//	unsigned char dt;
//	char *ptr = buffer;
//	Tick = 0;
//	while(Tick <= timeout){
//		dt = Receive_Data0(timeout);
//		if(dt == until) break;
//		*ptr = dt;
//		ptr+=1;
//	}
//	*ptr = '\0';
//}
void ReadStringUntil(unsigned char *buffer,unsigned char until, int max_length){
	int i;
	unsigned char dt;
	char *ptr = buffer;
	for(i=0;i<max_length;i++){
		dt = Receive_Data(UART1);
		if(dt == until) break;
		*ptr = dt;
		ptr+=1;
	}
	*ptr = '\0';
}
//void ReadStringUntil1(unsigned char *buffer,unsigned char *rev, int max_length){
//	int i;
//	unsigned char dt;
//	char *ptr = buffer;
//	for(i=0;i<max_length;i++){
//		dt = Receive_Data(UART1);
//		if(strstr(ptr,rev)!=NULL) break;
//		if(strstr(ptr,rev)=="ERR") break;
//		*ptr = dt;
//		ptr+=1;
//	}
//	*ptr = '\0';
//}


//int waitResponse1(unsigned char *buffer, unsigned char *expect_str,int timeout){
////	int i;
//	int index = 0;
//	unsigned char dt;
//	Tick = 0;
//	while (Tick < timeout){
//		//if(RI_1==1){
//		//while (!RI_1);
//			dt = Receive_Data(UART1);
//		UART2_Send_Data(dt);
//		if (dt <= 0) continue;
//		*buffer = dt;
//		buffer++;
//		//if(startMillis == 7 ) LED_GREEN = 0;
//		if (strstr(buffer, expect_str) !=NULL) {
//          index = 1;
//          //goto finish;
//					break;
//		} else if (strstr(buffer, "ERROR") !=NULL) {
//          index = 2;
//         // goto finish;
//					break;
//		}
//	}
//	//finish: 
//	//puts_UART2(buffer);
//	Tick = 0;
//		if(!index){
//			sprintf(M,"%s\r\n",bufffer);
//			puts_UART2(M);
//		}
//	return index;
//}
void GPIO_Init(){
	MODIFY_HIRC(HIRC_24);
	//ALL_GPIO_QUASI_MODE;
	//SCON_1 = 0;
	P12_PUSHPULL_MODE;
	BEEP_OFF;
	P31_QUASI_MODE;
	P35_QUASI_MODE;
	P36_QUASI_MODE;
	P37_QUASI_MODE;
	Set_Relay(0);
	//wifi
	P34_PUSHPULL_MODE;
	//power
	P32_PUSHPULL_MODE;
	P14_PUSHPULL_MODE;
	P22_PUSHPULL_MODE;
//	P32_QUASI_MODE;
//	P14_QUASI_MODE;
//	P22_QUASI_MODE;
	//led
	P04_PUSHPULL_MODE;
	P05_PUSHPULL_MODE;
	//sa
	P15_PUSHPULL_MODE;
	P16_PUSHPULL_MODE;
	P02_PUSHPULL_MODE;
	P25_PUSHPULL_MODE;
	//key
	P33_INPUT_MODE;
	P01_INPUT_MODE;
	P03_INPUT_MODE;
	//P32 P22 P14     P37 P36 P35 P31			P01  P33
//	P31_PUSHPULL_MODE;
//	P35_PUSHPULL_MODE;
//	P36_PUSHPULL_MODE;
//	P37_PUSHPULL_MODE;
//	Set_Relay(0);
//	RL1=0;
//	RL2=0;
//	RL3=0;
//	RL4=0;
	///uart 
	//0b10011111
	//AUXR2 = 0x9F;
	//SCON_1 |= 0x10;
	// uart2 - debug
	P30_QUASI_MODE;
  ENABLE_UART2_TXD_P30;  
  UART2_Open(24000000,115200); //Open uart2
//	DISABLE_UART2_INTERRUPT;
	
	
	//uart1 - sim
	P10_QUASI_MODE;
	P00_INPUT_MODE;
	ENABLE_UART1_TXD_P10;
	ENABLE_UART1_RXD_P00;
	UART_Open(24000000,UART1_Timer3,115200);
	
	DISABLE_UART1_INTERRUPT;
	
	//uart0 - esp
	P07_INPUT_MODE;
	P06_QUASI_MODE;
	UART_Open(24000000,UART0_Timer1,115200);
//	ENABLE_UART0_PRINTF;
	DISABLE_UART0_INTERRUPT;
	
	//test esp - sim
	//P10_INPUT_MODE;
	//P00_INPUT_MODE;
	
//		TIMER2_DIV_128;
//		TIMER2_DIV_512;
//    TIMER2_Auto_Reload_Delay_Mode;
//		//1s
////    RCMP2H = 0x48;
////    RCMP2L = 0xF5;
//    TL2 = 0x48;
//    TH2 = 0xF5;
//		//1ms
////		RCMP2H = 0xFF;
////    RCMP2L = 0x44;
////		TL2 = 0xFF;
////    TH2 = 0x44;
//		ENABLE_TIMER2_INTERRUPT;
//		ENABLE_GLOBAL_INTERRUPT;    
//    set_EIE_ET2;                                    // Enable Timer2 interrupt
//    set_IE_EA;
//    set_T2CON_TR2;  

//	ENABLE_TIMER1_MODE0;                           /* Timer 0 mode configuration */
//    TIMER0_FSYS_DIV12;

//    TH0 = 0x48;
//    TL0 = 0xF5;
//      
//    ENABLE_TIMER0_INTERRUPT;                       /* enable Timer0 interrupt  */ 
//    ENABLE_GLOBAL_INTERRUPT;                       /* enable interrupts */

//    set_TCON_TR0;  

	TIMER2_DIV_512;
    TIMER2_Auto_Reload_Delay_Mode;
  
    RCMP2H = 0x48;
    RCMP2L = 0xF5;

    TL2 = 0x48;
    TH2 = 0xF5;

    set_EIE_ET2;                                    // Enable Timer2 interrupt
    set_IE_EA;
    set_T2CON_TR2;
//		ENABLE_GLOBAL_INTERRUPT; 
		
		SET_INT_Timer2_LEVEL1;
		SET_INT_UART1_LEVEL2;
		SET_INT_UART0_LEVEL2;
		
}



int waitResponse1(unsigned char *expect_str,int timeout){
	Tick=0;
	update=0;
	ID = 0;
	do{
		if(update==0){
			update=2;
			ENABLE_UART1_INTERRUPT;
			
		} else if(update==1){
			update = 0;
				
			ID = 0;
			if(strstr(RxUART,expect_str) != NULL){
				DISABLE_UART1_INTERRUPT;
				return 1;
			}
			else if(strstr(RxUART,"ERR") != NULL){
				DISABLE_UART1_INTERRUPT;
				return 2;
			}
		}
		delay_ms(10);
//		Tick++;
	} while(Tick < timeout);
	DISABLE_UART1_INTERRUPT;
	return 0;
}
int waitResponse0(unsigned char *expect_str,int timeout){
	Tick=0;
	update=0;
	ID = 0;
	do{
		if(update==0){
			update=2;
			ENABLE_UART0_INTERRUPT;
			
		} else if(update==1){
			update = 0;
				
			ID = 0;
			if(strstr(RxUART,expect_str) != NULL){
				DISABLE_UART0_INTERRUPT;
				return 1;
			}
		}
		delay_ms(10);
	} while(Tick < timeout);
	DISABLE_UART0_INTERRUPT;
	return 0;
}
int waitResponse11(unsigned char *expect_str,int timeout){
	Tick=0;
	update=0;
	ID = 0;
	do{
		if(update==0){
			update=2;
			ENABLE_UART1_INTERRUPT;
			
		} else if(update==1){
			update = 0;
				
			ID = 0;
			if(strstr(RxUART,expect_str) != NULL){
				DISABLE_UART1_INTERRUPT;
				return 1;
			}
			else if(strstr(RxUART,"OK") != NULL){
				DISABLE_UART1_INTERRUPT;
				return 1;
			}
			else if(strstr(RxUART,"ERR") != NULL){
				DISABLE_UART1_INTERRUPT;
				return 2;
			}
		}
		delay_ms(10);
//		Tick++;
	} while(Tick < timeout);
	DISABLE_UART1_INTERRUPT;
	return 0;
}
void AutoEchoOFF(){
	memset(RxUART, '\0', 100);
	puts_UART1("ATE0\r\n");
	if(1 == waitResponse1("OK",20)){
		puts_UART2("EchoOFF OK");
	} else {
		puts_UART2("EchoOFF Fail");
	}
}
void GetInfo(){
	memset(RxUART, '\0', 100);
	puts_UART1("ATI\r\n");
	if(1 == waitResponse1("OK",50)){
		puts_UART2(RxUART);
	} else {
		puts_UART2("GetInfo Fail");
	}
}
void GetIMEI(){
	int i=0;
	puts_UART1("AT+GSN\r\n");
	ReadStringUntil(&bufffer,'\n',20);
	ReadStringUntil(&bufffer,'\n',20);
	for(i=0;i<15;i++){
		IMEI[i] = bufffer[i];
	}
	sprintf(M,"%s\n", bufffer);
	puts_UART2(M);
	delay_ms(20);
}
unsigned int readRSSI4G(){
	//AT+CSQ
	unsigned int ret = 101;
	char *ptr;
	int tmp;
	memset(RxUART, '\0', 100);
	puts_UART1("AT+CSQ\r\n");
	tmp = waitResponse11("CSQ:",5);
	if(1 == tmp){
		//puts_UART2(RxUART);
		ptr = strstr(RxUART, "+CSQ:");
		sscanf(ptr, "+CSQ: %d", &ret);
	}
	delay_ms(100);
	return ret;
}
void readCREG(){
	//AT+CSQ
	memset(RxUART, '\0', 100);
	puts_UART1("AT+CREG?\r\n");
	if(1 == waitResponse1("CREG",2)){
		puts_UART2(RxUART);
	}
}
void ConnectNetwork(){
	memset(RxUART, '\0', 100);
	puts_UART1("AT+CGATT=1\r\n");
	if(1 == waitResponse1("OK",5)){
		puts_UART2(RxUART);
	}
	memset(RxUART, '\0', 100);
	puts_UART1("AT+CGDCONT=1,\"IP\",\"dialogbb\"\r\n");
	if(1 == waitResponse1("OK",5)){
		puts_UART2(RxUART);
	}
	memset(RxUART, '\0', 100);
	puts_UART1("AT+CGACT=1,1\r\n");
	if(1 == waitResponse1("OK",5)){
		puts_UART2(RxUART);
	}
	memset(RxUART, '\0', 100);
	puts_UART1("AT+CGPADDR=1\r\n");
	if(1 == waitResponse1("CGPADDR",5)){
		puts_UART2(RxUART);
	}
	delay_ms(100);
}
void simReset(){
	puts_UART1("AT+QRST=1\r\n");
	if(1 != waitResponse1("OK",2)){
		puts_UART2(M);
	}
}
int MQTT_Open(){
	memset(RxUART, '\0', 100);
	puts_UART1("AT+QMTCFG=\"recv/mode\",0,0,1\r\n");
	if(1 != waitResponse1("OK",2)){
		//puts_UART2(RxUART);
		return 1;
	}
//	delay_ms(100);
//	memset(RxUART, '\0', 100);
//	puts_UART1("AT+QMTCFG=\"keepalive\",0,100\r\n");
//	if(1 != waitResponse("OK",2)){
//		return 2;
//	}
//	delay_ms(100);
//	memset(RxUART, '\0', 100);
//	puts_UART1("AT+QMTCFG=\"timeout\",0,60,10,1\r\n");
//	if(1 != waitResponse("OK",2)){
//		return 3;
//	}
	
//	puts_UART1("AT+QMTOPEN=0,");
//	puts_UART1(test_mqttServer);
//	puts_UART1(",");
//	//putc_UART1(test_mqttPort,1);
//	sprintf(M,"%d\r\n",test_mqttPort);
	memset(RxUART, '\0', 100);
	sprintf(M,"AT+QMTOPEN=0,%s,%d\r\n",test_mqttServer,test_mqttPort);
	puts_UART1(M);
	if(1 != waitResponse11("QMTOPEN:",2)){
		//puts_UART2(RxUART);
		return 4;
	}
	delay_ms(100);
//	puts_UART1("AT+QMTCONN=0,");
//	puts_UART1(ESP_ID);
//	puts_UART1(",");
//	puts_UART1(mqttUser);
//	puts_UART1(",");
//	puts_UART1(mqttPassword);
//	puts_UART1("\r\n");
//	puts_UART1("AT+QMTCONN=0,\"94E6862F1EA0\",\"lctech_dev\",\"lctech_DEV@123\"\r\n");
	memset(RxUART, '\0', 100);
	sprintf(M,"AT+QMTCONN=0,\"%s\",%s,%s\r\n",ESPID,mqttUser,mqttPassword);
	puts_UART1(M);
	if(1 != waitResponse11("QMTCONN:",2)){
		//puts_UART2(RxUART);
		return 5;
	}
	delay_ms(100);
	return 0;
}
int MQTT_Publish(){
	////869373068197954
//	int i=0;
	memset(RxUART, '\0', 100);
//	puts_UART1("AT+QMTSUB=0,1,\"mmm/94E6862F1EA0/cmd\",0\r\n");
	sprintf(M,"AT+QMTSUB=0,1,%s,0\r\n",txt_subscribe);
	puts_UART1(M);
	if(1 != waitResponse11("QMTSUB:",2)){
		//puts_UART2(RxUART);
		return 1;
	}
	delay_ms(100);
	//AT+QMTPUBEX=0,0,0,0,(char*)topic_srv,String(length)
//	puts_UART1("AT+QMTPUBEX=0,1,1,0,\"swm/gsafe4/data\",16\r\n");
	memset(RxUART, '\0', 100);
	sprintf(M,"AT+QMTPUBEX=0,1,1,0,%s,%d\r\n",topicSever,(int)69);
	puts_UART1(M);
	if(1 != waitResponse1(">",2)){
		//puts_UART2(RxUART);
		//return 2;
	}
//	Receive_Data(UART1);
	//send fdata to mqtt
//	putc_UART1(send,18);
	memset(RxUART, '\0', 100);
	puts_UART1(HexString);
	
	if(1 == waitResponse1("OK",2)){
		//return 3;
	}
	delay_ms(100);
//	puts_UART1("AT+QMTDISC=0\r\n");
	return 0;
}
char h2c(char c){  
  return "0123456789ABCDEF"[0x0F & (unsigned char)c];
}
uint8_t updateHexString(uint8_t value, uint8_t index){
  HexString[index++]=h2c(value>>4);
  HexString[index++]=h2c(value&0x0F);
  return(index);
}
void updateData2Sent(){
	uint8_t index = 0;
	int line = 0;
	int sum = 0;
	gdata.rssi4G = readRSSI4G();
//	gdata.rssi4G = 255;
	for(line=0;line<15;line++){
		dataload[index++] = IMEI[line];
	}
	for(line=0;line<12;line++){
		dataload[index++] = ESPID[line];
	}
	dataload[index++] = gdata.pwStatus; 
	dataload[index++] = gdata.batPercent;
	dataload[index++] = gdata.Status;
	dataload[index++] = gdata.rssi4G;
	dataload[index++] = gdata.rssiWF; 
	for(line = 0; line < 16; line++)
  {    
    dataload[index++]= gdata.lineStatus[line];
  }
	//hexstring
	index = 0;
	for(line = 0; line < 27; line++)
  {    
    HexString[index++] = dataload[line];
  }
	index = updateHexString(dataload[27],index);
	index = updateHexString(dataload[28],index);
	index = updateHexString(dataload[29],index);
	index = updateHexString(dataload[30],index);
	index = updateHexString(dataload[31],index);
	for(line = 32	; line < 48; line++)
  {    
    index = updateHexString(dataload[line],index);
  }
	for(line =0;line<69;line++){
		sum= sum+HexString[line];
	}
	HexString[index++] = (uint8_t)sum;
	HexString[index++] = '\n';
	HexString[index++] = '\0';
}
void send2ESP(){
	updateData2Sent();
	puts_UART0("Data:");
	puts_UART0(HexString);
	delay_ms(10);
}	

void getRssiWF(){
	char *ptr;
	memset(RxUART, '\0', 100);
	if(1==waitResponse0("WF",2)){
		ptr = strstr(RxUART, "WF:");
		sscanf(ptr, "WF:%d", &gdata.rssiWF);
//		sprintf(M,"rssiWWF: %d\n",gdata.rssiWF);
//		puts_UART2(M);
	}
	delay_ms(100);
}
void getESPID(){
	char *ptr;
	memset(RxUART, '\0', 100);
	if(1==waitResponse0("ID",5)){
		ptr = strstr(RxUART, "ID:");
		//ptr += 3;
		sscanf(ptr, "ID:%s", ESPID);
//		sscanf(ptr, "%s", ESPID);
	}
	delay_ms(100);
	sprintf(M,"ID:%s\r\n",ESPID);
	puts_UART2(M);
}
char power_status;
unsigned int RSSI = 1 ;
//----------------------------------------------------MAIN------------------------------------------------------//
int sp=0;
unsigned int IN16M;
void Send2Server(){
	if(MQTT_Open() == 0){
			puts_UART2("Open Mqtt 4G OK");
		} else {
			puts_UART2("Open Mqtt 4G False");
			}
		if(MQTT_Publish() == 0){
			puts_UART2("Send Mqtt 4G OK");
			gdata.rssiInit = 5;
			sp = 0;
		} else {
			puts_UART2("Send Mqtt 4G False");
			gdata.rssiInit +=5;
			}
}
void main (void)
{
	
	Tick = 0;
	GPIO_Init();
	DC_ON = 1;
	CH_OFF = 0; 
	EN_WIFI = 0;
	delay_ms(800);
	EN_WIFI = 1;
	delay_ms(10);
	getESPID();
	delay_ms(1000);
	GetIMEI();
	gdata.rssiWF = 255;
	gdata.enableBell = 0;
	gdata.rssiInit = 5;
	AutoEchoOFF();
	ConnectNetwork();
//------------------------------------------------WHILE(1)------------------------------------------------------//	
  while(1)
  {
		delay_ms(10);
		controlPower();
		IN16M = check_IN16M();
		checkAlarm();
		/* COMM WITH ESP */
		send2ESP(); 	//send to esp
//		sprintf(M,"RSSI:%d\n",gdata.rssiInit);	//send rssiInit to esp
//		puts_UART0(M);
		delay_ms(20);
		getRssiWF();	//read rssiWF from esp
		/* SELF */
		sprintf(M,"SP:%d    %d\n",(int)gdata.rssiInit,(int)sp);
		puts_UART2(M);
		if(gdata.rssiInit >  100) gdata.rssiInit =5;
		//send to sv
		if(gdata.rssiWF < 125 | gdata.rssiWF !=0){
			if(sp > 300){
				Send2Server();
				sp = 0;
			} 
		} else {
			if(gdata.Status == ALARM){
				if(sp >= 10){
					Send2Server();
				}
			}	else {
				if(sp >= 30){
					Send2Server();
				}
			}
		}
  }
}

void Timer2_ISR (void) interrupt 5
{
	
    PUSH_SFRS;
    clr_T2CON_TF2; 
		sp++;
		Tick++;
		readADC_16line();
		
		if(gdata.Status == ALARM){
			LED_GREEN = 1;
			LED_RED = ~LED_RED;
		} else {
			LED_RED = 1;
			LED_GREEN = ~LED_GREEN;
		}
    POP_SFRS;
}	
void SerialPort1_ISR(void) interrupt 15
{
  char dataIn;
	PUSH_SFRS;
		
    if (RI_1 == 1)
    {
			dataIn = SBUF_1;
        
        if(dataIn != '\r' & dataIn>0){
					RxUART[ID] = dataIn;
					ID++;
				}
				else if (dataIn == '\r' && ID>0)
				{
					update=1;
			//		sprintf(M,"%s\r\n",RxUART);
		//for(i=0;i<ID,i++)
		//			puts_UART2(M);
					
				}
			//uart1_receive_data = SBUF_1;
				clr_SCON_1_RI_1;                             /* clear reception flag for next reception */
        //uart1_receive_flag = 1;
    }

    if (TI_1 == 1)
    {
        if (!PRINTFG)
        {
            clr_SCON_1_TI_1;                             /* if emission occur */
        }
    }
    POP_SFRS;
} 
void Serial_ISR(void) interrupt 4
{
	char dataIn;
    PUSH_SFRS;
    if (RI)
    {
				dataIn = SBUF;
        
        if(dataIn != '\r' & dataIn>0){
					RxUART[ID] = dataIn;
					ID++;
				}
				else if (dataIn == '\r' && ID>0)
				{
					update=1;
					}
//				uart0_receive_flag = 1;
//        uart0_receive_data = SBUF;
        clr_SCON_RI;                                            // Clear RI (Receive Interrupt).
    }

    if (TI)
    {
//        if (!PRINTFG)
//        {
//            TI = 0;
//        }
    }
    POP_SFRS;
}  

