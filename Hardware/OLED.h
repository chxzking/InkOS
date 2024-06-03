#ifndef __OLED_H
#define __OLED_H
#include "stm32f10x.h"  
#include <stdio.h>
//���ص�򿪻��߹ر�
typedef enum BOLT{
	CLOSE,
	OPEN
}BOLT;

/******Ӳ������*******/
void OLED_Init(void);//��ʼ��
void OLED_Clear(void);//�����Ļ��ʾ
void Buffer_Output(void);//��ӡ����������
/******����������******/

//�ٿ�λ�����ص�(�����꣨0-63���������꣨0-127��)
int Pixel_Operate(unsigned char __ROW,unsigned char __Columns,BOLT Bolt);
//��ָ���Դ渳ֵ(�����꣨0-7���������꣨0-127��)
int Display_register(unsigned char __ROW,unsigned char __Columns,unsigned char Register_value);
//ˢ�»�����
int Buffer_Refresh(void);

//ԭ�Ӳ���������������ֵ�������Ļ��
void print(void);
//����ͼƬ����������ͼƬλ�ã�ͼƬ�����ݣ�ͼƬ�ĳ���
int Picture_Loading(signed char __ROW,signed char __Columns,unsigned char* picture,unsigned char picture_length,unsigned char picture_width);
//����
void Rectangle(signed char ROW,signed char Columns,unsigned char length, unsigned char width);

//��ģ��ȡ
int Font_Read(unsigned char ROW,unsigned char Columns,char serial_numbers);
//�ַ�����ӡ
void Hj_String(signed char __ROW,signed char __Columns,char* str);
//������ӡ
void Hj_Int(signed char __ROW,signed char __Columns,int num);
#endif
