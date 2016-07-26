#include "led.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "string.h"
#include "key.h"
#include "sim900.h"
#include "GPRS.h"

u8 judgeGPRSData(u8*);

extern u8 SMSREADTEST;
extern u8 FLAG_GPRS_READCMD;//GPRS������־
extern u8 FLAG_SMS_CMD;//���ŵ�����־
u8 mode;

int main(void)
 {u8 SMSFLAG=1,CheckFLAG=0;	
	u8 FLAG_GPRS_Init=0,FLAG_GPRS_EstablishTCP=0; //some flag value of GPRS
	u8 GPRS_RCVDATA[20]={0};
  u8 FLAG_GPRS_DATA_FORM=255;//�жϷ��ص�ָ�����ͣ�Ĭ��255
	u8 MAIN_FLAG_GPRS_CMD=0;//�������ж��Ƿ�������
	
	mode=2;
	FLAG_GPRS_READCMD=0;
	
	KEY_Init();
	delay_init();	    	 //��ʱ������ʼ��	
	NVIC_Configuration();// �����ж����ȼ�����
	uart_init(9600);	 //���ڳ�ʼ��Ϊ9600
	FLAG_GPRS_Init=SIM900_GPRS_Init();
	LED_Init();		  	 //��ʼ����LED���ӵ�Ӳ���ӿ� 
  LED0=1;
	LED1=1;
//Judge if GPRS_Init is successful
	switch(FLAG_GPRS_Init)
	{
		case 1://succeed
			LED0_RUN(4);
		   delay_ms(1000);
		  LED1_RUN(4);
		  break;
		case 2:
			LED0_RUN(3);
		   delay_ms(1000);
		  LED1_RUN(3);
		  break;
		case 3:	
		case 4:
		case 5:
		case 6:
			LED0_RUN(2);
		   delay_ms(1000);
		  LED1_RUN(2);
		  break;
		default:break;
	
	}
	//judge if eatablish TCP Link successful
  FLAG_GPRS_EstablishTCP=establishTCPLink();
  switch(FLAG_GPRS_EstablishTCP)
	{
		case 0://�޷���OK
			LED0_RUN(1);
		   delay_ms(1000);
		  LED1_RUN(1);
		  break;
		case 1://�޷���CONNECT OK
			LED0_RUN(2);
		   delay_ms(1000);
		  LED1_RUN(2);
		  break;
		case 2://�����ɹ�
			LED0_RUN(3);
		   delay_ms(1000);
		  LED1_RUN(3);
		  break;
		default:
			break;
	}
  	
	while(1)
	{ 
///////////////////////////////////////////////////////////////////////////////
		/*���Ŵ���*/
	  SMSREADTEST=SIM900_READ_TEXT_TEST(mode);	
		switch(SMSREADTEST)
		{
		      case 0:LED0_RUN(5);break;
					case 1:LED0_RUN(6);break;
					case 2:LED0_RUN(7);break;
			    default: break;		
		}
		SMSREADTEST=5;// mode=2;	
///////////////////////////////////////////////////////////////////////////////		
	//���Ƿ�GPRS�����ݵ���
			
		MAIN_FLAG_GPRS_CMD=GPRS_READ(GPRS_RCVDATA,FLAG_GPRS_READCMD);
			if((MAIN_FLAG_GPRS_CMD==1)||(MAIN_FLAG_GPRS_CMD==2))
		{ LED0_RUN(1);
			LED1_RUN(3);
		 FLAG_GPRS_DATA_FORM=judgeGPRSData(GPRS_RCVDATA);
			switch(FLAG_GPRS_DATA_FORM)
			{
			  case 0:LED0_RUN(4);
				   break;
				case 1:LED0_RUN(8);
				   break;
				default:break;
			
			}//switch end
			FLAG_GPRS_DATA_FORM=255;			
		}
		  MAIN_FLAG_GPRS_CMD=0;
		 
/////////////////////////////////////////////////////////////////////////////			
					
  	if(KEY_Scan(0)==WKUP_PRES)//�ж��Ƿ�������������LED1��
		{
		 CheckFLAG=SIM900_Check();
			if(CheckFLAG==1)
			{LED1_RUN(2);
			 CheckFLAG=0;
			}else LED1=1;
		}

	}//	 while end
}//main end

u8 judgeGPRSData(u8* rcvData)
{ int flag=0;
  if((rcvData[0]==0x31)&&(rcvData[1]==0x31)&&(rcvData[4]==0x32)&&(rcvData[8]==0x33)&&(rcvData[12]==0x34))//����������
{
  //LED0_RUN(8);//��8�
 return 1;
}
return 255;
}

