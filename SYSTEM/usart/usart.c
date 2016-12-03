#include "sys.h"
#include "usart.h"	  
#include "string.h"
#include "sim900.h"
#include "led.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"					//ucos 使用	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//串口1初始化		   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/8/18
//版本：V1.5
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved
//********************************************************************************
//V1.3修改说明 
//支持适应不同频率下的串口波特率设置.
//加入了对printf的支持
//增加了串口接收命令功能.
//修正了printf第一个字符丢失的bug
//V1.4修改说明
//1,修改串口初始化IO的bug
//2,修改了USART_RX_STA,使得串口最大接收字节数为2的14次方
//3,增加了USART_REC_LEN,用于定义串口最大允许接收的字节数(不大于2的14次方)
//4,修改了EN_USART1_RX的使能方式
//V1.5修改说明
//1,增加了对UCOSII的支持
////////////////////////////////////////////////////////////////////////////////// 	  
 

//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
    USART1->DR = (u8) ch;      
	return ch;
}
#endif 

/*使用microLib的方法*/
 /* 
int fputc(int ch, FILE *f)
{
	USART_SendData(USART1, (uint8_t) ch);

	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}	
   
    return ch;
}
int GetKey (void)  { 

    while (!(USART1->SR & USART_FLAG_RXNE));

    return ((int)(USART1->DR & 0x1FF));
}
*/
 

//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记	  

//初始化IO 串口1 
//bound:波特率
void uart_init(u32 bound){
    //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
	  USART_InitTypeDef USART_InitStructure;
	  NVIC_InitTypeDef NVIC_InitStructure;
	 
	  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
 	  USART_DeInit(USART1);  //复位串口1
	 //USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_mode = GPIO_mode_AF_PP;	//复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA9
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_mode = GPIO_mode_IN_FLOATING;//浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA10

   //Usart1 NVIC 配置

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级3
	  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	  NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
   //USART 初始化设置

	 USART_InitStructure.USART_BaudRate = bound;//一般设置为9600;
	 USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	 USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	 USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	 USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	 USART_InitStructure.USART_mode = USART_mode_Rx | USART_mode_Tx;	//收发模式

    USART_Init(USART1, &USART_InitStructure); //初始化串口
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断
    USART_Cmd(USART1, ENABLE);                    //使能串口 

}
/*****************************************************************************************************************/
/*                    This serial port segment only receive serial data                                          */
/*                    clear serial buffer function left to other relative segment in other files                 */
/*                    And GSM data definited by self-customized datagram ended with DA\r\n                       */
/*                    so it is convient to judge GSM data coming signal by this kind of end                      */
/*****************************************************************************************************************/

u8 Res;                  //serial port received data put into this variable
u8 i=0;                  //flag of data length to indicate received data
u8 FLAG_SM_Received=0;   //flag of small message coming,if set to 1 means A message come  
u8 FLAG_OK=0;            //ordinary OK flag indicates the right return OK
u8 FLAG_GSM_CONNECTED=0; //flag of GSM connection,if succeed to a server than set to 1. 
u8 SMSREADTEST=6;
extern u8 SMSLocationMode;       //SMS location flag,to judge location number of SMS,0 for less than 10 and 1 for more than 10
u8 FLAG_GSM_DATA_COMING;           //flag of GPRS data coming,changed by usart received data 
extern u8 FLAG_SMS_CMD;
/* 0x43->C,0x4d->M,0x4f->O   */
/* 0x54->T,0x49->I,0x47->G   */
/* 0x52->R ,0x4b->K          */
/* 0x2b->+,                  */
/* 0x44->D,0x41->A           */
extern u8 FLAG_WHEN_Execute_EstabTCPLink;	
#if EN_USART1_RX   //如果使能了接收
void USART1_IRQHandler(void)                	//串口1中断服务程序
	{
	char SIM900_SMSREAD[]="CMTI";
	char SIM900_OK[]="OK"; 
	char SIM900_CONNECTOK[]="CONNECT";
	char SIM900_DATA[]="DA";	
	SMSLocationMode=2;	
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART1, USART_IT_RXNE)!=RESET)//接收中断(接收到的数据必须是0x0d 0x0a结尾)
{
		     Res =USART_ReceiveData(USART1);//(USART1->DR);//读取接收到的数据
         USART_RX_BUF[i]=Res;
         i++;//stored data number increased by 1
    //to judge serial content,and ensure end by "enter"
 	  if((i>1)&&(USART_RX_BUF[i-1]==0x0a)&&(USART_RX_BUF[i-2]==0x0d))
   { 
		 if(FLAG_WHEN_Execute_EstabTCPLink==0)
	  { //This flag of FLAG_WHEN_Execute_EstabTCPLink to ensure returned OK is not CONNECT OK  
			//判断是否来到CONNECT oK 或普通OK,以下执行普通OK代码
			 if((USART_RX_BUF[i-1]==0x0a)&&(USART_RX_BUF[i-2]==0x0d)&&(USART_RX_BUF[0]!=0x2b)&&(USART_RX_BUF[1]!=0x43)&&(USART_RX_BUF[2]!=0x4d)&&(USART_RX_BUF[3]!=0x47)&&(USART_RX_BUF[4]!=0x52))//换行，但首句不是CMGR              
      {//这才是判断是否到来ＯＫ		，这是普通OK，非CONNECT OK	  
				 FLAG_OK=1;//variable set 
				 i=0;
			}else if((USART_RX_BUF[i-1]==0x0a)&&(USART_RX_BUF[i-2]==0x0d)&&(USART_RX_BUF[i-3]==0x4b)&&(USART_RX_BUF[i-4]==0x4f)&&(USART_RX_BUF[0]==0x2b)&&(USART_RX_BUF[1]==0x43)&&(USART_RX_BUF[2]==0x4d)&&(USART_RX_BUF[3]==0x47)&&(USART_RX_BUF[4]==0x52))//结尾以OK\r\n结束�  以+CMGR开始//////////////////////////////////////////////                                                                                                                   �
			     {/*结尾以OK\r\n结束, 以+CMGR开*/
						 i=0;		  
		       }else if((USART_RX_BUF[i-1]==0x0a)&&(USART_RX_BUF[i-2]==0x0d)&&(USART_RX_BUF[1]==0x43)&&(USART_RX_BUF[2]==0x4d)&&(USART_RX_BUF[3]==0x54)&&(USART_RX_BUF[4]==0x49))//接收短信CMTI,结尾以换行结shu  ///////////////////////////////////////////////////////////////////////////////////////////////////
			             { //接收短信CMTI,结尾以换行结shu  //
							   			i=0;
			                FLAG_SM_Received=1;	//There is message coming,flag is set.
                    // FLAG_SMS_CMD=1;				
			             }else if(((USART_RX_BUF[i-1]==0x0a)&&(USART_RX_BUF[i-2]==0x0d)&&(USART_RX_BUF[i-4]==0x4f)&&(USART_RX_BUF[i-3]==0x4b)))//接收到OK\r\n
			                  {//接收到OK\r\n
				                 
				                  FLAG_OK=1;	
				                  //  LED0_RUN(2);  
				                  FLAG_GSM_DATA_COMING=1;
			                    i=0;
			                  }else if((USART_RX_BUF[i-3]==0x41)&&(USART_RX_BUF[i-4]==0x44))//DA\r\n
			                        {/*self-customized datagram ended by DA\r\n that indicates GSM data coming*/
																// GPRS有数据传过来
				                       //LED1_RUN(1);
				                         i=0;
				                         FLAG_GSM_DATA_COMING=1;
			                        }
	   }else if((USART_RX_BUF[i-1]==0x0a)&&(USART_RX_BUF[i-2]==0x0d)&&(USART_RX_BUF[i-12]==0x43)&&(USART_RX_BUF[i-11]==0x4f)&&(USART_RX_BUF[i-10]==0x4e))          
			   {/*THIS IF-ELSE SEGMENT EXECUTE WHEN ESTABLISH tcp LINK TO IDENTIFY ok AND coNNECT OK*/
					/*judge if it is end with CONNECT OK\r\n,to judge whether it is frist connected to server*/        
		        i=0;
				    FLAG_GSM_CONNECTED=1;
				    FLAG_WHEN_Execute_EstabTCPLink=0;//reset to 0 
			   }		
/*strstr(str1,str2)用于判断str2是否是str1的子串并返回首次出现的位置，未找到返回false                           */
/*SIM900_SMSREAD[]="CMTI"                                                                                      */
    if((strstr((const char*)USART_RX_BUF,(const char*)SIM900_SMSREAD)!=NULL)&&(FLAG_SM_Received==1))
		  {/*this segment handle Small Message Service*/
				LED0_RUN(2);//串口工作则红灯闪烁2下
				if((USART_RX_BUF[13]==0x0d)&&(USART_RX_BUF[14]==0x0a)) SMSLocationMode=0;//个位短信条数在十条以内
				else if((USART_RX_BUF[14]==0x0d)&&(USART_RX_BUF[15]==0x0a)) SMSLocationMode=1;//十位，短信条数在十条以上。
		    else SMSLocationMode=2;
				FLAG_SM_Received=0;
				i=0;			
      }else if((strstr((const char*)USART_RX_BUF,(const char*)SIM900_OK)!=NULL)&&(FLAG_OK==1))
			      { /*This segment handle ordinary OK*/
							//如果接收OK\r\n				
				      //LED0_RUN(2);
				      i=0;
				      FLAG_OK=0;
			      }else if((strstr((const char*)USART_RX_BUF,(const char*)SIM900_CONNECTOK)!=NULL)&&(FLAG_GSM_CONNECTED==1))
			            {/*This segment handle GSM connection succeed*/
										//receive CONNECT OK
			               i=0;
				             FLAG_GSM_CONNECTED=0;
				             //LED0_RUN(4);
			            }
		
}else if(i>=50)
      {/*receive more than 50 byte and not include OK\r\n*/
	      i=0;
      }
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntExit();  											 
#endif
} 
#endif	
}
