#include "bsp_adc.h"
#include "OSC.h"
#include "bsp_usart.h"
#include "delay.h"


/**
  * @brief  ADC GPIO 初始化
  * @param  无
  * @retval 无
  */
static void ADCx_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// ADCx_1 GPIO 初始化
	ADCx_1_GPIO_APBxClock_FUN(ADCx_1_GPIO_CLK, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = ADCx_1_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(ADCx_1_PORT, &GPIO_InitStructure);
	
	// ADCx_2 GPIO 初始化
	ADCx_2_GPIO_APBxClock_FUN(ADCx_2_GPIO_CLK, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = ADCx_2_PIN1;//第一个通道
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(ADCx_2_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = ADCx_2_PIN2;//第二个通道
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(ADCx_2_PORT, &GPIO_InitStructure);
}

/**
  * @brief  配置ADC工作模式
  * @param  无
  * @retval 无
  */
static  void ADCx_Mode_Config(void)
{
	ADC_InitTypeDef ADC_InitStructure;
	
	//配置ADC1工作模式
	ADCx_1_APBxClock_FUN(ADCx_1_CLK, ENABLE);
	
	ADC_InitStructure.ADC_Mode = ADC_Mode_RegSimult;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = NOFADCx_1_CHANEL;
	
	ADC_Init(ADCx_1, &ADC_InitStructure);
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div4);//14MHz
	ADC_RegularChannelConfig(ADCx_1, ADCx_1_CHANNEL, 1, ADC_SampleTime_1Cycles5);//转换时间1us
	
	ADC_ITConfig(ADCx_1, ADC_IT_EOC, ENABLE);
	ADC_Cmd(ADCx_1, ENABLE);
	
  // 初始化ADC1 校准寄存器  
	ADC_ResetCalibration(ADCx_1);
	// 等待校准寄存器初始化完成
	while(ADC_GetResetCalibrationStatus(ADCx_1));	
	// ADC开始校准
	ADC_StartCalibration(ADCx_1);
	// 等待校准完成
	while(ADC_GetCalibrationStatus(ADCx_1));
	
	ADC_SoftwareStartConvCmd(ADCx_1, ENABLE);
	
	
	
	//配置ADC2工作模式
	ADCx_1_APBxClock_FUN(ADCx_2_CLK, ENABLE);
	
	ADC_InitStructure.ADC_Mode = ADC_Mode_RegSimult;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = NOFADCx_1_CHANEL;
	
	ADC_Init(ADCx_2, &ADC_InitStructure);
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);//12MHz
	
		
	//使能ADCx_2的外部触发转换
  ADC_ExternalTrigConvCmd(ADCx_2, ENABLE);
	
	ADC_ITConfig(ADCx_2, ADC_IT_EOC, ENABLE);
	ADC_Cmd(ADCx_2, ENABLE);
	
  // 初始化ADC1 校准寄存器  
	ADC_ResetCalibration(ADCx_2);
	// 等待校准寄存器初始化完成
	while(ADC_GetResetCalibrationStatus(ADCx_2));
	
	// ADC开始校准
	ADC_StartCalibration(ADCx_2);
	// 等待校准完成
	while(ADC_GetCalibrationStatus(ADCx_2));	
}


//static void ADCx_NVIC_Config(void)
//{
//	NVIC_InitTypeDef NVIC_InitStructure;
//	//优先级分组
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
//	
//	NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQ;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
//}

void ADCx_Init(void)
{
	//ADCx_NVIC_Config();
	ADCx_GPIO_Config();
	ADCx_Mode_Config();
}


FlagStatus Get_Trigger_Status(float d0, float d1)
{
	if(TriggerModeNrb == 0)
	{
		if((d0 > CurTriggerValue) && (d1 <= CurTriggerValue))
		{
			return SET;
		}
	}
	else if(TriggerModeNrb == 1)
	{
		if((d1 >= CurTriggerValue) && (d0 < CurTriggerValue))
			{
				return SET;
			}
	}	
	return RESET;
}



void Get_Wave(void* parameter)
{
	uint8_t   PreSample=100, flag = 1;//波形数据采集完成标志位
	uint16_t  ADC_SampleCount = 0;
	float d0, d1;
	
	while(1)
	{
		ADC_SampleCount = 0;
		
		while(PreSample--)//丢弃前面20个
		{
			while(ADC_GetITStatus(ADCx_1, ADC_IT_EOC) == RESET);
			d0 = ADC_GetConversionValue(ADCx_1)/4096.0*3.3;
			ADC_ClearITPendingBit(ADCx_1, ADC_IT_EOC);	
		}
		
		//等待触发
		if(SamplingModeNrb != 0)
		{
			do
			{
				while(ADC_GetITStatus(ADCx_1, ADC_IT_EOC) == RESET);
				d0 = ADC_GetConversionValue(ADCx_1)/4096.0*3.3;
				ADC_ClearITPendingBit(ADCx_1, ADC_IT_EOC);		
				while(ADC_GetITStatus(ADCx_1, ADC_IT_EOC) == RESET);
				d1 = ADC_GetConversionValue(ADCx_1)/4096.0*3.3;
				ADC_ClearITPendingBit(ADCx_1, ADC_IT_EOC);
			}while(Get_Trigger_Status(d0, d1) != SET);
		}
		
		//开始采样
		while(ADC_SampleCount < ADCx_1_SampleNbr)
		{
			while(ADC_GetITStatus(ADCx_1, ADC_IT_EOC) != SET);
			ADC_ConvertedValue[ADC_SampleCount] = ADC_GetConversionValue(ADCx_1)*198/4096;//将采样值映射到显示区间
			if((CurTimePerDiv/50 -1) != 0 && (CurTimePerDiv/50 -1) <= 1000)
			{
				rt_hw_us_delay( CurTimePerDiv/50 -1 );//采样间隔时间
			}
			else
			{
				rt_thread_delay( (CurTimePerDiv/50)/1000 );//采样间隔时间
			}			
			ADC_ClearITPendingBit(ADCx_1, ADC_IT_EOC);
			ADC_SampleCount++;
		}
		if(SamplingModeNrb == 2)//如果是单次模式则挂起波形采样线程
		{
			rt_mq_send(getwave_status_queue, &flag, sizeof(flag));
			SamplStatusNrb = 0;
			CurSamplStatus = SamplStatus[SamplStatusNrb];
			Setting_Inf_Update(0);
			rt_thread_suspend(GetWave_thread);
		}
		rt_mq_send(getwave_status_queue, &flag, sizeof(flag));
	}	
}


uint16_t Get_X_Data(void)
{
	uint16_t XData = 0;
	ADC_RegularChannelConfig(ADCx_2, ADCx_2_X_CHANNEL, 1, ADC_SampleTime_71Cycles5);//转换时间7us
	ADC_SoftwareStartConvCmd(ADCx_2, ENABLE);
	
	while(ADC_GetITStatus(ADCx_2, ADC_IT_EOC) != SET);
	XData = ADC_GetConversionValue(ADCx_2);
	ADC_ClearITPendingBit(ADCx_2, ADC_IT_EOC);
	return XData;
}


uint16_t Get_Y_Data(void)
{
	uint16_t YData = 0;
	ADC_RegularChannelConfig(ADCx_2, ADCx_2_Y_CHANNEL, 1, ADC_SampleTime_71Cycles5);//转换时间7us
	ADC_SoftwareStartConvCmd(ADCx_2, ENABLE);
	
	while(ADC_GetITStatus(ADCx_2, ADC_IT_EOC) != SET);
	YData = ADC_GetConversionValue(ADCx_2);
	ADC_ClearITPendingBit(ADCx_2, ADC_IT_EOC);
	return YData;
}


