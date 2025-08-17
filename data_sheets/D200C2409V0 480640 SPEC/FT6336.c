/******************** (C) COPYRIGHT 2015**************************************
* �ļ�����         : MSG2233.c
* ����             : 
* �汾             : V1.1
* ����             : 22/9/2015
* ����             : �ַ����������ʾ
********************************************************************************
* ע����� 
*
*
*******************************************************************************/

#include "MSG2233.h"
#include "IIC_TP.h"
#include "sys_nvic.h"
#include "funcdata.h"

#define X_LENGTH 854
#define Y_LENGTH 480

static TP_POINT tpPoint;
//tp��ȡ����״̬
UINT8 Tp_PressStaus = 0;
//����״̬�µ�ͨѶ��Ŀ
static u16 TpComOutCount;
static u16 TpComOutFlag;

void MSG2233Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);//ʹ��GPIOCʱ��


	/*-------------------------------------------------------------*/
//	#define MSG_INT PBin(2)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;// 
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��GPIO
	

	EXTI_InitStructure.EXTI_Line=EXTI_Line14 ;	//ѡ���ж���·
	EXTI_InitStructure.EXTI_Mode=EXTI_Mode_Interrupt;  //����Ϊ�ж�����
	EXTI_InitStructure.EXTI_Trigger=EXTI_Trigger_Falling;//EXTI_Trigger_Rising; //�����жϴ�����ʽΪ�½��ش���
	EXTI_InitStructure.EXTI_LineCmd=ENABLE;
	EXTI_Init(&EXTI_InitStructure);	
	

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource14);	    //ѡ���жϹܽ�
	
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitStructure.NVIC_IRQChannel=EXTI15_10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x02;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x02;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	
		/*-------------------------------------------------------------*/
//	#define MSG_RST PBout(7)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;// 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//��ͨ���ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��GPIO
	
	MSG_RST=1;
//	delay_ms(1);
//	MSG_RST=0;
//	delay_ms(10);
//	MSG_RST=1;

	TpComOutCount = 0;
		
}

#define CTP_WRITE             				0x70
#define CTP_READ             					0x71
#define CTP_ACK_COUNTER        			10//150

#define TD_STAT_ADDR 0x2
#define TT_MODE_BUFFER_INVALID 0x08
#define TD_STAT_NUMBER_TOUCH 0x07

void CTP_I2C_delay()
{
	delay_us(1);
}
U8 CTP_I2C_READ(U8 reg, U8 *value, U8 len) 
{
	U8 i;
	IIC_TP_Start();  
	CTP_I2C_delay();
	IIC_TP_Send_Byte(CTP_WRITE);
	if(IIC_TP_Wait_Ack()!=0)			//�ȴ�Ӧ��
	{
		IIC_TP_Stop();
		return  0;						//Ӧ��ʧ��
	}
	CTP_I2C_delay();
	IIC_TP_Send_Byte(reg);
	if(IIC_TP_Wait_Ack()!=0)			//�ȴ�Ӧ��
	{
		IIC_TP_Stop();
		return  0;						//Ӧ��ʧ��
	}
	CTP_I2C_delay();
	IIC_TP_Stop();
	CTP_I2C_delay();
	IIC_TP_Start();
	IIC_TP_Send_Byte(CTP_READ);
	if(IIC_TP_Wait_Ack()!=0)			//�ȴ�Ӧ��
	{
		IIC_TP_Stop();
		return  0;						//Ӧ��ʧ��
	}
	CTP_I2C_delay();
	if (len <= 1)
    {
        *value =IIC_TP_Read_Byte(0);
		CTP_I2C_delay();
    }
	else
	{
	    for (i = 0; i< len - 1; i++)
	    {
	        *value++ =IIC_TP_Read_Byte(1);
			CTP_I2C_delay();
	    }
		*value =IIC_TP_Read_Byte(0);
		CTP_I2C_delay();
	}
    IIC_TP_Stop();
 	 
    return 1;
}
U8 CTP_I2C_Write(U8 reg, U8 value)
{
	IIC_TP_Start();  
	CTP_I2C_delay();
	IIC_TP_Send_Byte(CTP_WRITE);
	if(IIC_TP_Wait_Ack()!=0)			//�ȴ�Ӧ��
	{
		IIC_TP_Stop();
		return  0;						//Ӧ��ʧ��
	}
	CTP_I2C_delay();
	IIC_TP_Send_Byte(reg);
	if(IIC_TP_Wait_Ack()!=0)			//�ȴ�Ӧ��
	{
		IIC_TP_Stop();
		return  0;						//Ӧ��ʧ��
	}
	CTP_I2C_delay();
	IIC_TP_Send_Byte(value);
	if(IIC_TP_Wait_Ack()!=0)			//�ȴ�Ӧ��
	{
		IIC_TP_Stop();
		return  0;						//Ӧ��ʧ��
	}
	CTP_I2C_delay();
	IIC_TP_Stop();
	return 1;
}

void CTP_Init(void)
{
	U8 i;
	U8 lvalue;

	//����CTP zhouwei
//	CTP_I2C_Write(0x80, 0x1e); 
//	CTP_I2C_READ(0x80, &lvalue, 1);
	
	do{
		 CTP_I2C_READ(0xa3, &lvalue, 1);
		 i++;
		 delay_ms(50);
		 if((lvalue==0x06)||(lvalue==0x36)||(lvalue==0x64))
		 	break;
	  }
	while(i<20);
	if((lvalue==0x06)||(lvalue==0x36)||(lvalue==0x64))
	{

	}
	delay_ms(10);
//	CTP_I2C_Write(0x1B, 2);
}

#define TOUCH1_XH_ADDR 0x03
#define TOUCH2_XH_ADDR 0x09
#define TOUCH3_XH_ADDR 0x0F
#define TOUCH4_XH_ADDR 0x15
U8 values[4];
void CTP_GetTpOnePoint(const U8 x_base,TP_POINT *point,U8 *tpStu)
{
	
	CTP_I2C_READ(x_base, values, 4);
	if(x_base == 0x3)
	*tpStu = (values[0]>>6)&0x3;//(values[0]&0xC0);//
	point->x = (((U16)(values[0]&0x0f))<<8) | values[1];
	point->y =   (((U16)(values[2]&0x0f))<<8) | values[3];
}
U8 CTP_GetTpPoint(TP_POINT *Tmp_point,U8 *TpStu)
{
	U8 counter = 0,i;
	U8 lvalue;
	U8 PointsNum;
	TP_POINT tpPoint;
	int Tp_Point_x=0;
	int Tp_Point_y=0;
	const U8 x_base[] = {TOUCH1_XH_ADDR, TOUCH2_XH_ADDR, TOUCH3_XH_ADDR, TOUCH4_XH_ADDR};
	do{ //make sure data in buffer is valid
		CTP_I2C_READ(TD_STAT_ADDR, &lvalue, 1);
		if(counter++ == 0x30)
		{
		return 0;
		}
	}while(lvalue & TT_MODE_BUFFER_INVALID); 	

	CTP_I2C_READ(TD_STAT_ADDR, &lvalue, 1); 
	PointsNum = (U8)(lvalue & TD_STAT_NUMBER_TOUCH);
	if((PointsNum == 0) || (PointsNum > 5))
		PointsNum = 1;
	for(i=0;i<PointsNum;i++)//
	{
		CTP_GetTpOnePoint(x_base[i], &tpPoint,TpStu);
		Tp_Point_x += tpPoint.x;
		Tp_Point_y += tpPoint.y;
	}
	Tmp_point->x = Tp_Point_x/PointsNum;
	Tmp_point->y = Tp_Point_y/PointsNum;
	return 1;
}

/***********************************************************************
* ��������    : TP_SpeedAdd
* ����        : ͨѶ��ʱ�����Լ�
* �����β�    :
* ����ֵ      :
************************************************************************/
void TP_ComCountAdd(void)
{
	if(TpComOutCount >0)
	{
		TpComOutCount--;	
		if(TpComOutCount == 0)
		{
			TpComOutFlag = 1;
			//��������
			Touch_Int();
		}
	}
}

/***********************************************************************
* ��������    : ReadTpComOutFlag
* ����        : ͨѶ��ʱ��ʶ
* �����β�    : ��
* ����ֵ      : ��
************************************************************************/
UINT16 ReadTpComOutFlag(void)
{
	return TpComOutFlag;
}
/***********************************************************************
* ��������    : ClearTpComOutFlag
* ����        : ���ͨѶ��ʱ��ʶ
* �����β�    : ��
* ����ֵ      : ��
************************************************************************/
void ClearTpComOutFlag(void)
{
	TpComOutFlag = 0;
}
U8 TpStuV = 0;
//��ȡ����
U8  ReadTpPoint(void)
{
	int Tp_Point_x=0;
	int Tp_Point_y=0;

	TP_POINT tptmpPoint;	
	TpStuV = 0;
	if(CTP_GetTpPoint(&tptmpPoint,&TpStuV))
	{
			if(TpStuV == 1)
			{
				//�˴�������ָ�ɿ�
				if(Tp_PressStaus ==TP_Stu_Press)
				{
				Tp_PressStaus = TP_Stu_DoPress;
				}
				TpComOutCount = 0;
				return 1;
			}
			Tp_Point_x=tptmpPoint.x;//�õ�X
			Tp_Point_y=tptmpPoint.y;//�õ�y				
			
			if(Tp_PressStaus == TP_Stu_NoPress)
			{
				//����ɴ�����
				tpPoint.x=Tp_Point_y;
				tpPoint.y=Tp_Point_x;
			}
			
			Tp_PressStaus = TP_Stu_Press;

			return 1;
	}
	else
	{
			//У��ʧ��
			Tp_Point_x=-1;
			Tp_Point_y=-1;
			return 0;
	}
}

void  EXTI15_10_IRQHandler(void)
{
	#ifdef OS_TICKS_PER_SEC	 	//���ʱ�ӽ�����������,˵��Ҫʹ��ucosII��.
		OSIntEnter();	 
	#endif
	if(EXTI_GetITStatus(EXTI_Line14)!=RESET)
	{
		TpComOutCount = 35;
		//��������
		Touch_Int();
		EXTI_ClearITPendingBit(EXTI_Line14);
	}
	#ifdef OS_TICKS_PER_SEC	 	//���ʱ�ӽ�����������,˵��Ҫʹ��ucosII��.
		OSIntExit();											 
	#endif

}


/***********************************************************************
* ��������    : ReadTpPressStu
* ����        : ��ȡTp״̬
* �����β�    : ��
* ����ֵ      : ��
************************************************************************/
UINT8 ReadTpPressStu(void)
{
	return Tp_PressStaus;
}
/***********************************************************************
* ��������    : SetTpPressStu
* ����        : ���ð���״̬
* �����β�    : ��
* ����ֵ      : ��
************************************************************************/
void ClearTpPressStu(void)
{
	Tp_PressStaus = TP_Stu_NoPress ;
}
/***********************************************************************
* ��������    : getTPPoint
* ����        : ��ȡ����
* �����β�    : ��
* ����ֵ      : ��
************************************************************************/
void getTPPoint(TP_POINT *point)
{

		point->x = tpPoint.x;
		point->y = tpPoint.y;

//	point->x = tpPoint.x;
//	point->y = tpPoint.y;
}

