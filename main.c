#include "MDR32F9Qx_rst_clk.h"
#include "MDR32F9Qx_eeprom.h"
#include "MDR32F9Qx_port.h"
#include "MDR32F9Qx_arinc429r.h"
#include "MDR32F9Qx_arinc429t.h"
#include "lib_linear_buffer.h"

PORT_InitTypeDef PORT_InitStructure; //для инициализации портов

extern uint32_t ReadBytes;
uint32_t ReadArray[16];
extern Linear_buffer_organization_variables_t LINEAR_BUF;  
Linear_buffer_organization_variables_t LINEAR_BUF; 
//Linear_buffer_organization_variables_t *pLINEAR_BUF = &LINEAR_BUF;   

int main(void)
{
   ARINC429R_InitChannelTypeDef ARINC429R_InitChannelStruct;  //структуры для инициализации
   ARINC429T_InitChannelTypeDef ARINC429T_InitChanelStruct;
	 //Linear_buffer_organization_variables_t LINEAR_BUF; 
	// Linear_buffer_organization_variables_t *pLINEAR_BUF = &LINEAR_BUF;
	
	 uint8_t err=0;
	 err=serial_init(&LINEAR_BUF);
	 if (err!=0)
		 while(1);
		
	
	uint32_t TestData[16];
	uint32_t i;
	//ReadBytes=0;
	
	//---------------------------------------------------------

	 //тактирование HSE
    RST_CLK_HSEconfig(RST_CLK_HSE_ON);
    if (RST_CLK_HSEstatus() == ERROR)
    {
        while (1);
    }
    //умножение
    RST_CLK_CPU_PLLconfig(RST_CLK_CPU_PLLsrcHSEdiv1, RST_CLK_CPU_PLLmul10);  //80
    //разрешение
    RST_CLK_CPU_PLLcmd(ENABLE);
    if (RST_CLK_CPU_PLLstatus() == ERROR)
    {
        while (1);
    }
    //тактирование ПЗУ
    RST_CLK_PCLKcmd(RST_CLK_PCLK_EEPROM, ENABLE);
    //?
    EEPROM_SetLatency(EEPROM_Latency_3);
    // исп pll
    RST_CLK_CPU_PLLuse(ENABLE);
    //делитель
    RST_CLK_CPUclkPrescaler(RST_CLK_CPUclkDIV1);
    //путь от с3
    RST_CLK_CPUclkSelection(RST_CLK_CPUclkCPU_C3);
		
	//-------------------------------------------------------
		
		//тактирование переферии
		RST_CLK_PCLKcmd(RST_CLK_PCLK_ARINC429R, ENABLE);  //для ДПК
		RST_CLK_PCLKcmd(RST_CLK_PCLK_ARINC429T, ENABLE);
		RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTD, ENABLE);   //для портов B, D
		RST_CLK_PCLK2cmd(RST_CLK_PCLK2_PORTH, ENABLE);
		
	//-------------------------------------------------------
		
		//настройка портов для ДПК
		PORT_DeInit(MDR_PORTH);
		PORT_DeInit(MDR_PORTD);
		PORT_InitStructure.PORT_Pin = PORT_Pin_2 | PORT_Pin_3;
    PORT_InitStructure.PORT_FUNC  = PORT_FUNC_ALTER;
    PORT_InitStructure.PORT_SPEED = PORT_SPEED_MAXFAST;
    PORT_InitStructure.PORT_MODE  = PORT_MODE_DIGITAL;
		PORT_InitStructure.PORT_PD  = PORT_PD_DRIVER;
    PORT_Init(MDR_PORTB, &PORT_InitStructure);
    PORT_InitStructure.PORT_Pin = PORT_Pin_10 | PORT_Pin_11;
		PORT_Init(MDR_PORTH, &PORT_InitStructure);
		
	//------------------------------------------------------
	
	  //настройка ДПК
		//для канала 1 IN 
		ARINC429R_DeInit();     //сброс
    ARINC429R_BRG_Init(80); //делитель
    ARINC429R_InitChannelStruct.ARINC429R_CLK = ARINC429R_CLK_100_KHz;  //частота приема данных, если DIV !=0
    ARINC429R_InitChannelStruct.ARINC429R_SD = DISABLE;  //декодирование 9,10 бита данных (распознание источника/приемника)
    ARINC429R_InitChannelStruct.ARINC429R_LB = DISABLE;  //распознавание метки, бит 1-8
    ARINC429R_InitChannelStruct.ARINC429R_SDI1 = RESET;  //бит сравнения?
    ARINC429R_InitChannelStruct.ARINC429R_SDI2 = RESET;
    ARINC429R_InitChannelStruct.ARINC429R_DIV = 0;       //делитель для опорной частоты
    ARINC429R_ChannelInit(ARINC429R_CHANNEL2, &ARINC429R_InitChannelStruct);
		ARINC429R_ITConfig((ARINC429R_IT_INT_DR|ARINC429R_IT_INT_ER), ENABLE); //прерывание если в буфере приемника не пусто и по ощибки скорости, паузы | ARINC429R_IT_INT_ER 
    NVIC_EnableIRQ(ARINC429R_IRQn);           //разрешить прерывание в регистре
		ARINC429R_ChannelCmd(ARINC429R_CHANNEL2, ENABLE);  //разрешить работу первого канала
		
		// для канала 1(OUT)
		ARINC429T_DeInit();      //сброс
    ARINC429T_BRG_Init(80);  //делитель
    ARINC429T_InitChanelStruct.ARINC429T_CLK = ARINC429T_CLK_100_KHz;  //частота передачи
    ARINC429T_InitChanelStruct.ARINC429T_DIV = 0;            //делитель для опорной частоты
    ARINC429T_InitChanelStruct.ARINC429T_EN_PAR = ENABLE;    //бит паритета
    ARINC429T_InitChanelStruct.ARINC429T_ODD = ARINC429T_ODD_ADD_TO_ODD;  //бит формируется как дополнение к нечетному
    ARINC429T_ChannelInit(ARINC429T_CHANNEL1, &ARINC429T_InitChanelStruct);
    ARINC429T_ChannelCmd(ARINC429T_CHANNEL1, ENABLE);   //разрешить работу 1 канала
		while(1)
		{
		for(i = 0; i < 16; i++) 
    {
			  TestData[i]=i;   //заполняем массив
       ARINC429T_SendData(ARINC429T_CHANNEL1,TestData[i]);  //отправляем из массива
			 
     }
	 }
//while(1){}
    /*for(i = 0; i < 16; i++)
		 {
			 ReadArray[i]=get_serial(&LINEAR_BUF);
     }*/
 }

void ARINC429R_IRQHandler(void)   //обработчик прерываний
{
        uint32_t i=0;     //счетчик
	      uint32_t temp;  //промежуточная для загрузки в буфер
        bool st=0;
	       //Linear_buffer_organization_variables_t LINEAR_BUF; 

        ARINC429R_SetChannel(ARINC429R_CHANNEL2);
       while(ARINC429R_GetFlagStatus(ARINC429R_CHANNEL2, ARINC429R_FLAG_DR) == SET) //пока буфер не пустой вsчитываем данные
        {
					for(i = 0; i < 16; i++) 
          {
						temp = ARINC429R_ReceiveData();
						st=put_serial(&LINEAR_BUF, temp); 
				    if (st!=true)
		        while(1);
					}						
        }
     
}

