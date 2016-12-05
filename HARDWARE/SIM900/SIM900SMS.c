/*����ĵ��Ǳ���Ŀ�Ķ��Ž�����֤����*/
#include "SIM900SMS.H"
#include "usart.h"
#include "sys.h"
#include "delay.h"
#include "led.h"
#include "string.h"
#include "stdio.h"

 char SIM900_OK[]="OK"; 
 char SIM900_AT[]="AT";
 char SIM900_CSCS[]="AT+CSCS=\"GSM\"";
 char SIM900_SMSRETURN[]=">";
 char SIM900_SMSREAD[]="+CMTI";
 char SIM900_SMSREADCONT[]="CMGR";
 char SIM900_TEST[]="TEST";
 char SIM900_READTEXTCMD[]="AT+CMGR=";

 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*                  ����ATָ��ж�SIM900�����Ƿ�����                                  */
u8 SIM900_Check(void)
{Clear_USART_RX_BUF();	
 printf("%s\r\n",SIM900_AT);
 delay_ms(1500);
 if(strstr((const char*)USART_RX_BUF,(const char*)SIM900_OK))
  {
    Clear_USART_RX_BUF();	
    return 1;//return OK
  }
  return 0;	 //error
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u8 SIM900_SendSMS()
{ u8 count=0;//flag varial to statistic waiting time by sum
 /* ���AT+CSCS=\"GSM\"\r\n"���ж��Ƿ�OK��3�����򱨴�*/
	printf("AT+CSCS=\"GSM\"\r\n");
	/*                 wait until time out                                             */
	while((strstr((const char*)USART_RX_BUF,(const char*)SIM900_OK))==NULL)
	{ delay_ms(100);
		count++;
		if(count>30)
		{
			return 0;//no expected return
		}
	}
	Clear_USART_RX_BUF();
	count=0;
	/*����AT+CMGF=1\r\n���ж��Ƿ�OK��*/
	printf("AT+CMGF=1\r\n");
	while((strstr((const char*)USART_RX_BUF,(const char*)SIM900_OK))==NULL)
	{ delay_ms(100);
		count++;
		if(count>30)
		{ 
			return 1;
		}
	}
	
  Clear_USART_RX_BUF();
	count=0;
	
	printf("AT+CMGS=\"18020484363\"\r\n");// set destination phone number
	while((strstr((const char*)USART_RX_BUF,(const char*)SIM900_SMSRETURN)==NULL))
	{delay_ms(100);
	 count++;
		if(count>50)
		 { 
			 return 2; 
		 }
	}
printf("Congratulation!!");//send successfully
	delay_ms(3000);
USART_SendData(USART1,0X1A);
while(USART_GetFlagStatus(USART1,USART_FLAG_TXE)==RESET);	
	LED0=0;
	
return 3;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////??
/*                           �������,�ɲ���                                                                                   */
u8 SIM900_Read_Text()
{ u8 HighBit,LowBit;
	
	u8 count=0;
	u8 SMContentArray[20]={0};
	u8 i=0;
	
 if(strstr((const char*)USART_RX_BUF,(const char*)SIM900_SMSREAD))//�ж��Ƿ�+CMTI���ж��ŵ���
 {
   HighBit=USART_RX_BUF[12];//����λ��   ʮλ
	 LowBit=USART_RX_BUF[13];//����λ��    ��λ
	 /*���Ͷ��Ŷ�ȡָ��*/
	 printf("AT+CMGR=");
	 USART_SendData(USART1,HighBit);
	 USART_SendData(USART1,LowBit);
	 printf("\r\n");
   
	 while(strstr((const char*)USART_RX_BUF,(const char*)SIM900_SMSREADCONT)==NULL)
	 { delay_ms(100);
		 count++;
		 if(count>40)
		 {
		  count=0;
			 return 0;
		 }
	 }
   delay_ms(200);
	 
   /*��ȡ����ʵ�����ݣ�����INFO64����*/	
	 for(i=0;i<20;i++)
	 {
	  SMContentArray[i]=USART_RX_BUF[64+i];
	 
	 }
	 
	
 }else return 0;

return 1;
}
/*               above is the test code                                                                   */
/*****************************************************************************************************************************/


/************************************************************************************************************************************/
/*��ȡ���� ʵ������  */
extern u8 SMSLocationMode;//�жϽ��ն��ŵ�����λ�ã�10Ϊ�ָ���
u8 SMContentArray[20]={0};//�������ʵ������
u8 FLAG_SMS_CMD=0;//�ж��Ƿ���ŷ���
u8 SIM900_READ_SM(u8 SMSLocationMode)
{ u8 count=0;
	
	u8 i=0;
	//u8 flag=9;//�ж϶���λ��
	u8 FLAG_SM_CHECKFORMAT=0;//check the SM format to ensure correction of content
	u8 HighBit;//λ�ø�λ
	u8 LowBit;//λ�õ�λ
	u8 Hex[10]={0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9};//mask,to extract the factual data
	
	if(SMSLocationMode==1)
		{//SM length more than 10
      HighBit=USART_RX_BUF[12];
	    LowBit=USART_RX_BUF[13];
	    HighBit=Hex[HighBit-0x30];
	    LowBit=Hex[LowBit-0x30];
	    SMSLocationMode=2;                      //reset SMS location to error bit
	    Clear_USART_RX_BUF();
	    printf("%s%x%x\r\n\r\n",SIM900_READTEXTCMD,HighBit,LowBit);//send reading SMS instructor
	}else if(SMSLocationMode==0)
	{//SMS location less than 10
	    HighBit=USART_RX_BUF[12];
	    HighBit=Hex[HighBit-0x30];	
      SMSLocationMode=2;	
	    Clear_USART_RX_BUF();
	    printf("%s%x\r\n\r\n",SIM900_READTEXTCMD,HighBit);
	}else 
  {
		  return 9;
	}

   while(strstr((const char*)USART_RX_BUF,(const char*)SIM900_SMSREADCONT)==NULL)// ��ȡ����ʵ������,
{ //JUDGE THE CONTENT BY CMGR,NOT IN THE SERIAL PORT SEGMENT                     // 10���޷���CMGR��return 0
     delay_ms(100);
		 count++;
		 if(count>100)
		 {
		   count=0;
			 return 0;//stop at this stage,return 0 means led flash 5 times
		 }
}
   delay_ms(200);

 /*                 This stage means having got through check stage,Read the SM actual content              */
	 for(i=0;i<20;i++)
	 {
	   //SMContentArray[i]=USART_RX_BUF[64+i];
	   SMContentArray[i]=USART_RX_BUF[4+i];
		 if(i==19) 
		 FLAG_SM_CHECKFORMAT=1;//6��
	 }

 /*                 check the preset format of SM,  ����Ƿ�11** 2*** 3*** 4****                              */	 
  if((SMContentArray[4]==0x32)&&(SMContentArray[8]==0x33)&&(SMContentArray[12]==0x34))//����������
{//(SMContentArray[0]==0x31)&&(SMContentArray[1]==0x31)&&
   //LED0_RUN(8);//��8��
   FLAG_SM_CHECKFORMAT=2;//7��
}
  return FLAG_SM_CHECKFORMAT;
}


/*********************************************************************************************************************************/
/*                                                     clear the data buffer                                                    */
void Clear_USART_RX_BUF()
{ u8 i=0;
  for(i=0;i<30;i++)
	USART_RX_BUF[i]=0;
}



