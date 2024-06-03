#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "inkos.h"
#include "heap_solution_1.h"

#define ONLY_ALLOWED_TO_SEND		2		//����������Ϣ
#define ONLY_ALLOWED_RECEPTION		1		//�����������Ϣ

#define QUEUE_ENABLE 	1			//��������
#define QUEUE_DISABLE	0			//���в�����

typedef struct Queue_Node Queue_Node;

struct Queue_Node{//���еĽڵ�
	unsigned char Count;//��ͬ��������
	void* Data;	//���������
	Queue_Node* next;//��һ�����нڵ�
};


struct Queue_t{//���е�ͷ
	unsigned int Queue_Max_Count;//���е����ڵ���
	unsigned int Current_Count;//���е�ǰ��Ϣ����
	unsigned int Size;//���ݵĳ���
	unsigned char SR_Flag;//�շ�Ȩ�ޱ�־��00��һλΪ����Ȩ�ޣ��ڶ�λΪ����Ȩ��
	unsigned char enable_flag;//�������ñ�־
	TASK_ORBIT_RING* Sender;//������
	TASK_ORBIT_RING* Receiver;//������
	Queue_Node* Read_Head;//��ͷ�����ڶ�ȡ������Ϣ
	Queue_Node* Queue_Tail;//���ڼ�¼����β�����������Ƿ���ϲ���Ϣ�����ٴ洢��
};




#endif
