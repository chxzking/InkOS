#include "stm32f10x.h"// Device header
//#include "Delay.h"
//#include "string.h"
#include "stdlib.h"
#include "five_way_key.h"
//#include "OLED.h"

KEY_DIRECTION KeyNum = KEY_EMPTY;

void FiveWayKey_Init(void){
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|
								  GPIO_Pin_2|
								  GPIO_Pin_3|
								  GPIO_Pin_4|
								  GPIO_Pin_5|
								  GPIO_Pin_6|
								  GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	
}


/*
1:��λ��
2��set��
3���м��
4���Ҽ�
5�����
6���¼�
7���ϼ�
*/

uint8_t FiveKey_Num(void){
	uint8_t key_flag = 1;
	
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1)==0){
		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1)==0);
		KeyNum = KEY_RESET;//reset
		return key_flag;
	}
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_2)==0){
		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_2)==0);
		KeyNum = KEY_SET;//set
		return key_flag;
	}
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_3)==0){
		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_3)==0);
		KeyNum = KEY_MID;//�м��
		return key_flag;
	}		
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_4)==0){
		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_4)==0);
		KeyNum = KEY_RIGHT;//�Ҽ�
		return key_flag;
	}	
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_5)==0){
		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_5)==0);
		KeyNum = KEY_LEFT;//���
		return key_flag;
	}	
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_6)==0){
		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_6)==0);
		KeyNum = KEY_DOWN;//�¼�
		return key_flag;
	}	
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_7)==0){
		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_7)==0);
		KeyNum = KEY_UP;//�ϼ�
		return key_flag;
	}
	key_flag = 0;
	return key_flag;
}



//char* FiveKey_char(void){
//	char* Keychar = (char*)malloc(6);
//	
//	
//	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1)==0){
//		
//		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1)==0);
//	
//		strcpy(Keychar,"reset");
//		//reset
//	}
//	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_2)==0){
//	
//		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_2)==0);
//	
//		strcpy(Keychar,"set");//set
//	}
//	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_3)==0){
//	
//		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_3)==0);
//	
//		strcpy(Keychar,"mid");//�м��
//	}		
//	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_4)==0){
//		
//		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_4)==0);
//	
//		strcpy(Keychar,"right");//�Ҽ�
//	}	
//	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_5)==0){

//		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_5)==0);

//		strcpy(Keychar,"left");//���
//	}	
//	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_6)==0){

//		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_6)==0);

//		strcpy(Keychar,"down");//�¼�
//	}	
//	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_7)==0){

//		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_7)==0);

//		strcpy(Keychar,"up");//�ϼ�
//	}
//	

//	
//	return Keychar;
//}
