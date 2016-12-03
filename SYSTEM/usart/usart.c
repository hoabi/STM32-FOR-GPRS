#include "sys.h"
#include "usart.h"	  
#include "string.h"
#include "sim900.h"
#include "led.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//���ʹ��ucos,����������ͷ�ļ�����.
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"					//ucos ʹ��	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������
//����1��ʼ��		   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/8/18
//�汾��V1.5
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved
//********************************************************************************
//V1.3�޸�˵�� 
//֧����Ӧ��ͬƵ���µĴ��ڲ���������.
//�����˶�printf��֧��
//�����˴��ڽ��������.
//������printf��һ���ַ���ʧ��bug
//V1.4�޸�˵��
//1,�޸Ĵ��ڳ�ʼ��IO��bug
//2,�޸���USART_RX_STA,ʹ�ô����������ֽ���Ϊ2��14�η�
//3,������USART_REC_LEN,���ڶ��崮�����������յ��ֽ���(������2��14�η�)
//4,�޸���EN_USART1_RX��ʹ�ܷ�ʽ
//V1.5�޸�˵��
//1,�����˶�UCOSII��֧��
////////////////////////////////////////////////////////////////////////////////// 	  
 

//////////////////////////////////////////////////////////////////
//�������´���,֧��printf����,������Ҫѡ��use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
_sys_exit(int x) 
{ 
	x = x; 
} 
//�ض���fputc���� 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//ѭ������,ֱ���������   
    USART1->DR = (u8) ch;      
	return ch;
}
#endif 

/*ʹ��microLib�ķ���*/
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
 

//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
u8 USART_RX_BUF[USART_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USART_RX_STA=0;       //����״̬���	  

//��ʼ��IO ����1 
//bound:������
void uart_init(u32 bound){
    //GPIO�˿�����
    GPIO_InitTypeDef GPIO_InitStructure;
	  USART_InitTypeDef USART_InitStructure;
	  NVIC_InitTypeDef NVIC_InitStructure;
	 
	  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//ʹ��USART1��GPIOAʱ��
 	  USART_DeInit(USART1);  //��λ����1
	 //USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_mode = GPIO_mode_AF_PP;	//�����������
    GPIO_Init(GPIOA, &GPIO_InitStructure); //��ʼ��PA9
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_mode = GPIO_mode_IN_FLOATING;//��������
    GPIO_Init(GPIOA, &GPIO_InitStructure);  //��ʼ��PA10

   //Usart1 NVIC ����

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//��ռ���ȼ�3
	  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	  NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
   //USART ��ʼ������

	 USART_InitStructure.USART_BaudRate = bound;//һ������Ϊ9600;
	 USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	 USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	 USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	 USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	 USART_InitStructure.USART_mode = USART_mode_Rx | USART_mode_Tx;	//�շ�ģʽ

    USART_Init(USART1, &USART_InitStructure); //��ʼ������
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//�����ж�
    USART_Cmd(USART1, ENABLE);                    //ʹ�ܴ��� 

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
#if EN_USART1_RX   //���ʹ���˽���
void USART1_IRQHandler(void)                	//����1�жϷ������
	{
	char SIM900_SMSREAD[]="CMTI";
	char SIM900_OK[]="OK"; 
	char SIM900_CONNECTOK[]="CONNECT";
	char SIM900_DATA[]="DA";	
	SMSLocationMode=2;	
#ifdef OS_TICKS_PER_SEC	 	//���ʱ�ӽ�����������,˵��Ҫʹ��ucosII��.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART1, USART_IT_RXNE)!=RESET)//�����ж�(���յ������ݱ�����0x0d 0x0a��β)
{
		     Res =USART_ReceiveData(USART1);//(USART1->DR);//��ȡ���յ�������
         USART_RX_BUF[i]=Res;
         i++;//stored data number increased by 1
    //to judge serial content,and ensure end by "enter"
 	  if((i>1)&&(USART_RX_BUF[i-1]==0x0a)&&(USART_RX_BUF[i-2]==0x0d))
   { 
		 if(FLAG_WHEN_Execute_EstabTCPLink==0)
	  { //This flag of FLAG_WHEN_Execute_EstabTCPLink to ensure returned OK is not CONNECT OK  
			//�ж��Ƿ�����CONNECT oK ����ͨOK,����ִ����ͨOK����
			 if((USART_RX_BUF[i-1]==0x0a)&&(USART_RX_BUF[i-2]==0x0d)&&(USART_RX_BUF[0]!=0x2b)&&(USART_RX_BUF[1]!=0x43)&&(USART_RX_BUF[2]!=0x4d)&&(USART_RX_BUF[3]!=0x47)&&(USART_RX_BUF[4]!=0x52))//���У����׾䲻��CMGR              
      {//������ж��Ƿ����ϣ�		��������ͨOK����CONNECT OK	  
				 FLAG_OK=1;//variable set 
				 i=0;
			}else if((USART_RX_BUF[i-1]==0x0a)&&(USART_RX_BUF[i-2]==0x0d)&&(USART_RX_BUF[i-3]==0x4b)&&(USART_RX_BUF[i-4]==0x4f)&&(USART_RX_BUF[0]==0x2b)&&(USART_RX_BUF[1]==0x43)&&(USART_RX_BUF[2]==0x4d)&&(USART_RX_BUF[3]==0x47)&&(USART_RX_BUF[4]==0x52))//��β��OK\r\n�����  ��+CMGR��ʼ//////////////////////////////////////////////                                                                                                                   �
			     {/*��β��OK\r\n����, ��+CMGR��*/
						 i=0;		  
		       }else if((USART_RX_BUF[i-1]==0x0a)&&(USART_RX_BUF[i-2]==0x0d)&&(USART_RX_BUF[1]==0x43)&&(USART_RX_BUF[2]==0x4d)&&(USART_RX_BUF[3]==0x54)&&(USART_RX_BUF[4]==0x49))//���ն���CMTI,��β�Ի��н�shu  ///////////////////////////////////////////////////////////////////////////////////////////////////
			             { //���ն���CMTI,��β�Ի��н�shu  //
							   			i=0;
			                FLAG_SM_Received=1;	//There is message coming,flag is set.
                    // FLAG_SMS_CMD=1;				
			             }else if(((USART_RX_BUF[i-1]==0x0a)&&(USART_RX_BUF[i-2]==0x0d)&&(USART_RX_BUF[i-4]==0x4f)&&(USART_RX_BUF[i-3]==0x4b)))//���յ�OK\r\n
			                  {//���յ�OK\r\n
				                 
				                  FLAG_OK=1;	
				                  //  LED0_RUN(2);  
				                  FLAG_GSM_DATA_COMING=1;
			                    i=0;
			                  }else if((USART_RX_BUF[i-3]==0x41)&&(USART_RX_BUF[i-4]==0x44))//DA\r\n
			                        {/*self-customized datagram ended by DA\r\n that indicates GSM data coming*/
																// GPRS�����ݴ�����
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
/*strstr(str1,str2)�����ж�str2�Ƿ���str1���Ӵ��������״γ��ֵ�λ�ã�δ�ҵ�����false                           */
/*SIM900_SMSREAD[]="CMTI"                                                                                      */
    if((strstr((const char*)USART_RX_BUF,(const char*)SIM900_SMSREAD)!=NULL)&&(FLAG_SM_Received==1))
		  {/*this segment handle Small Message Service*/
				LED0_RUN(2);//���ڹ���������˸2��
				if((USART_RX_BUF[13]==0x0d)&&(USART_RX_BUF[14]==0x0a)) SMSLocationMode=0;//��λ����������ʮ������
				else if((USART_RX_BUF[14]==0x0d)&&(USART_RX_BUF[15]==0x0a)) SMSLocationMode=1;//ʮλ������������ʮ�����ϡ�
		    else SMSLocationMode=2;
				FLAG_SM_Received=0;
				i=0;			
      }else if((strstr((const char*)USART_RX_BUF,(const char*)SIM900_OK)!=NULL)&&(FLAG_OK==1))
			      { /*This segment handle ordinary OK*/
							//�������OK\r\n				
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
#ifdef OS_TICKS_PER_SEC	 	//���ʱ�ӽ�����������,˵��Ҫʹ��ucosII��.
	OSIntExit();  											 
#endif
} 
#endif	
}
