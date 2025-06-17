#include "ms51_32k.h"

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