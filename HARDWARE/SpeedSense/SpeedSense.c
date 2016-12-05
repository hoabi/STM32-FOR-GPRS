#include "SpeedSense.h"


TIM_ICInitTypeDef  TIM2_ICInitStructure;

void TIM2_Cap_Init(u16 arr,u16 psc)
{	 
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
 	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);	//ʹ��TIM2ʱ��
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  //ʹ��GPIOAʱ��
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0;  //PA0 ���֮ǰ����  
	GPIO_InitStructure.GPIO_mode = GPIO_mode_IPD; //PA0 ����  
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA,GPIO_Pin_0);						 //PA0 ����
	
	//��ʼ����ʱ��2 TIM2	 
	TIM_TimeBaseStructure.TIM_Period = arr; //�趨�������Զ���װֵ 
	TIM_TimeBaseStructure.TIM_Prescaler =psc; 	//Ԥ��Ƶ��   
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_Countermode = TIM_Countermode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); //����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
  
	//��ʼ��TIM2���벶�����
	  TIM2_ICInitStructure.TIM_Channel = TIM_Channel_1; //CC1S=01 	ѡ������� IC1ӳ�䵽TI1��
  	TIM2_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;	//�����ز���
  	TIM2_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; //ӳ�䵽TI1��
  	TIM2_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;	 //���������Ƶ,����Ƶ 
  	TIM2_ICInitStructure.TIM_ICFilter = 0x00;//IC1F=0000 ���������˲��� ���˲�
  	TIM_ICInit(TIM2, &TIM2_ICInitStructure);
	
	//�жϷ����ʼ��
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;     //TIM2�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  //��ռ���ȼ�2��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  //�����ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;     //IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);                     //����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ��� 
	
	TIM_ITConfig(TIM2,TIM_IT_Update|TIM_IT_CC1,ENABLE);//��������ж� ,����CC1IE�����ж�	
	
  TIM_Cmd(TIM2,ENABLE ); 	//ʹ�ܶ�ʱ��2
 
}

/*                                       get the actual frequency                                                  */
u16 Counter_Now_Captured=0;//current counter of register when captured
u16 FLAG_Capture=0;        //to judge first or second time to capture high rising
u16 SUM_TIMER_COUNTER=0;   //
u16 FREQUERY_SUM=0;        //
extern u16 FREQUERY_SIHGLE;//current freq
//��ʱ��2�жϷ������	 
void TIM2_IRQHandler(void)
{ 
		if (TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)//Timer2 Update interrupt not occur
	{	    
		   TIM_ClearITPendingBit(TIM2,TIM_IT_CC1);
		    if(FLAG_Capture==0)//first time to capture high rising
		{
			  Counter_Now_Captured=TIM_GetCapture1(TIM2);
			  FLAG_Capture=1;		//Has captured first high rising
			
		} else if(FLAG_Capture==1)
			{
				if(TIM2,TIM_GetITStatus(TIM2,TIM_IT_Update)!=SET)//timer2 update interrupt not occur
				{
					 SUM_TIMER_COUNTER=TIM_GetCapture1(TIM2);//capture current value
				   SUM_TIMER_COUNTER=SUM_TIMER_COUNTER-Counter_Now_Captured;		
				} else
				{/*Timer update interrupt occur*/
				   TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
				   SUM_TIMER_COUNTER=0xffff+TIM_GetCapture1(TIM2)-Counter_Now_Captured+1;	
				}
			     FLAG_Capture=0;
			}
			FREQUERY_SIHGLE=1000000/SUM_TIMER_COUNTER;
    /*	intended to sum the frequency by 50	times
			if(SUM_TIMER_COUNTER!=0)//amount of timer counter between two high rising
		 {
		   FREQUERY_SIHGLE=1000000/SUM_TIMER_COUNTER;
		   flag++;
		   FREQUERY_SUM=FREQUERY_SUM+FREQUERY_SIHGLE;
		 }
		 if(flag==50)
		 {
			 flag=0;
	     TalFr=(int)FREQUERY_SUM/50;
		   FREQUERY_SUM=0;
		 }
     */
	}
   TIM_ClearITPendingBit(TIM2, TIM_IT_CC1|TIM_IT_Update); //����жϱ�־λ
   TIM_Cmd(TIM2,DISABLE);                                 //disable  interrupt unless set able
}

extern u8 FLAG_SENSESPEED_CMD;
void setSenseSpeedCmd()
{
  if(FLAG_SENSESPEED_CMD==1)
	 TIM_Cmd(TIM2,ENABLE);
}
