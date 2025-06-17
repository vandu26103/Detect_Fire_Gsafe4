/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* SPDX-License-Identifier: Apache-2.0                                                                     */
/* Copyright(c) 2020 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

#include "ms51_32k.h"

bit PRINTFG = 0, uart0_receive_flag = 0, uart1_receive_flag;
unsigned char uart0_receive_data, uart1_receive_data;








/*MS51 new version buadrate */
void UART_Open(unsigned long u32SysClock, unsigned char u8UARTPort, unsigned long u32Baudrate)
{
     SFRS = 0;
     switch (u8UARTPort)
    {
        case UART0_Timer1:
            SCON = 0x50;            //UART0 Mode1,REN=1,TI=1
            TMOD |= 0x20;           //Timer1 Mode1
            set_PCON_SMOD;          //UART0 Double Rate Enable
            set_CKCON_T1M;
            clr_T3CON_BRCK;          //Serial port 0 baud rate clock source = Timer1
            TH1 = 256 - (u32SysClock / 16 / u32Baudrate);
            set_TCON_TR1;
            set_IE_ES;
            break;

        case UART0_Timer3:
            SCON = 0x50;          //UART0 Mode1,REN=1,TI=1
            set_PCON_SMOD;        //UART0 Double Rate Enable
            T3CON &= 0xF8;        //T3PS2=0,T3PS1=0,T3PS0=0(Prescale=1)
            set_T3CON_BRCK;        //UART0 baud rate clock source = Timer3
            RH3    = HIBYTE(65536 - (u32SysClock / 16 / u32Baudrate));
            RL3    = LOBYTE(65536 - (u32SysClock / 16 / u32Baudrate));
            set_T3CON_TR3;         //Trigger Timer3
            set_IE_ES;
            break;

        case UART1_Timer3:
            SCON_1 = 0x50;          //UART1 Mode1,REN_1=1,TI_1=1
            T3CON = 0x80;           //T3PS2=0,T3PS1=0,T3PS0=0(Prescale=1), UART1 in MODE 1
            RH3    = HIBYTE(65536 - (u32SysClock/16/u32Baudrate));  
            RL3    = LOBYTE(65536 - (u32SysClock/16/u32Baudrate));     
            set_T3CON_TR3;             //Trigger Timer3
            set_EIE1_ES_1;
            break;
    }
 }
extern int Tick;
unsigned char Receive_Data(unsigned char UARTPort)
{
    UINT8 c;
		//unsigned char c;
		PUSH_SFRS;
    SFRS = 0;
		
    switch (UARTPort)
    {
        case UART0:
						while (!RI);
            c = SBUF;
            RI = 0;
            break;

        case UART1:
            while (!RI_1);
            c = SBUF_1;
            RI_1 = 0;	
            break;
    }
		POP_SFRS;
    return (c);
}

void UART_Send_Data(unsigned char UARTPort, unsigned char c)
{
    PUSH_SFRS;
    SFRS = 0;
    switch (UARTPort)
    {
        case UART0:
          TI=0;
          SBUF = c;
          while(!TI);
        break;
        case UART1:
          TI_1=0;
          SBUF_1 = c;
          while(!TI_1);//
        break;
    }
    POP_SFRS;
}


void Enable_UART0_VCOM_printf_24M_115200(void)
{
    P06_QUASI_MODE;
    UART_Open(24000000,UART0_Timer1,115200);
    ENABLE_UART0_PRINTF;
    DISABLE_UART0_INTERRUPT;
}
