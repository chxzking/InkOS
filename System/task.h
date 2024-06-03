#pragma once
#ifndef _TASK_H_
#define _TASK_H_

#include "inkos.h"
#include "inkos_config.h"

#if HEAP_SCHEME == HEAP_SOLUTION_1  
#include "heap_solution_1.h"
#endif

#include "bit_operate.h"

//���ݽṹ����
typedef struct TaskControlBlock TaskControlBlock;				                //����TCBģ��
//typedef struct TASK_ORBIT_RING TASK_ORBIT_RING;				                    //������������
typedef struct HashNode HashNode;                                               //�����ϣ�ڵ����ݽṹ
typedef struct HashTable HashTable;                                             //�����ϣ�����ݽṹ
//typedef struct SCHEDULING_PRIORITY_CHAIN SCHEDULING_PRIORITY_CHAIN;             //����ƻ���
//typedef struct INSPECTOR INSPECTOR;                                             //Ѳ��Ա

typedef enum TASK_STATUS TASK_STATUS;                                           //����״̬ö������

//���������ض���
//typedef void (*TASK_FUNCTION)(void*);							                //������
typedef  unsigned char TACTICS;                                                 //���Խṹ
//typedef STACK_DEPTH_TYPE StackDepth;                                            //ջ������ض���
typedef STACK_T_TYPE stack_t;                                                   //ջ�����ض���

//����SPC�ڵ���������
#define SPC_TYPE_HEADER     0                                                   //ͷ�ڵ�����
#define SPC_TYPE_GENERAL    1                                                   //��ͨ�ڵ�����

#define PRIORITY_MASK       0x0f                                                //���ȼ���׼����

//����ָ��TOR��״̬
#define STATUS_MASK         0xc0                                                //״̬����
#define STATUS_BASIS_CODE   0x40                                                //״̬��׼�޸���
#define TOR_ENABLE_FLAG     1                                                   //TOR����
#define TOR_DISABLE_FLAG    0                                                   //TOR������

//�����������־
#define NEW_TASK_AVAILABLE      1                                                   //������������־
#define NEW_TASK_NOT_AVAILABLE  0                                                   //û������������־

 //����״̬

//���ݽṹ����
/*TCB������ƿ鶨��*/
struct TaskControlBlock {
    stack_t* stack_top;                                                         //ջ��
    stack_t* stack_bottom;                                                      //ջ��
    StackDepth stack_size;                                                      //��ջ��С
    TASK_FUNCTION taskFunction;                                                 //������
    void* Args;                                                                 //����
};
/*TOR������������*/
struct TASK_ORBIT_RING {
    //����ײ�
    TaskControlBlock TCB;                                                       //TCB
    TASK_ORBIT_RING* self_handle;                                               //������
    SCHEDULING_PRIORITY_CHAIN* self_SPC;                                        //��ǰ���ص�SPC
    TACTICS tactics;                                                            //���ԣ�������ŴӸ�λ����λ������[0]-[1]����״̬��[2]-[3]������[4]-[7]���ȼ���
    unsigned char Task_Enable_Flag;                                             //�������ñ�־
    //�����ϲ�
    unsigned char* icon;                                                        //����logo����ģ  
    //����ָ����
    TASK_ORBIT_RING* next;                                                      //��һ�������ָ��
    TASK_ORBIT_RING* previous;                                                  //ǰһ�������ָ��
    //����������
    char Task_name[TASK_NAME_SIZE + 1];                                         //��������
};
/*SPC���ȼ��������������*/
struct SCHEDULING_PRIORITY_CHAIN {
    TASK_ORBIT_RING* Task_Ring;                                                 //���񻷹��ص㣨ͷ�ڵ���NULL��
    unsigned char Ring_Priority;                                                //�������ȼ� ��ͷ�����0��
    unsigned char Ring_Task_Amount;                                             //����ע��������������ͷ�ڵ�ͳ��SPC��������
    unsigned char Ring_Active_Amount;                                           //���е������ڻ�Ծ״̬������������̬��������  (ͷ����д���ʵ��ͳ�Ʒǿ�SPC�����������ǻ���ʵ�ִ���) 
    unsigned char SPC_Type;                                                     //���ȼ��������ڵ����ͣ�1Ϊ��ͨ���ͣ�0Ϊͷ��㣩
    TASK_ORBIT_RING* TOR_Leader;                                                //TOR���еĻ��ױ��
    SCHEDULING_PRIORITY_CHAIN* next;                                            //(ͷ����У�����洢SPC��һ���ڵ�ĵ�ַ)
    SCHEDULING_PRIORITY_CHAIN* previous;                                        //(ͷ����У���������洢SPC���һ���ڵ�ĵ�ַ)
};
//Ѳ��Ա����
struct INSPECTOR {
    SCHEDULING_PRIORITY_CHAIN* SPC;                                             //SPC
    TASK_ORBIT_RING* TOR;                                                       //TOR
};
/*��������ö�ٶ���*/
enum TASK_STATUS {
    TASK_OBSTRUCT,//����״̬
    TASK_READY,//����״̬
    TASK_RUNTIME,//����״̬
    TASK_ERROR//����ֵ
};
/*��ϣ��ڵ㶨��*/
struct HashNode {
    TASK_ORBIT_RING* TOR;  // ָ��TOR��ָ��,�����˴洢��������
    HashNode* next;  // �����ͻ��ʹ������
};
/*��ϣ����*/
struct HashTable {
    HashNode* array[HASH_TABLE_SIZE];  // ��ϣ������
    int size;  // ��ϣ��ʵ�ʴ洢��Ԫ������
};

///*ջ����ź�����*/
//enum Stack_Overflow_t{
//	STACK_OVERFLOW = -1,//ջ���
//	STACK_SAFETY = 0,	//ջ��ȫ
//	STACK_FULL = 1		//ջ������������߽磩
//};

/*************************
* Ѳ��Ա
**************************/
//��ʼ��Ѳ��
void Inspector_Init(void);
//������ȫѲ��(Ѳ��Ա���̸���Ϊһ���ڵ�,�����ظ�ֵ)
INSPECTOR Complete_Inspector_Forword(void);
//������ȫѲ��(Ѳ��Ա���̻��˵���һ���ڵ�,���ظ�ֵ)
INSPECTOR Complete_Inspector_Reverse(void);
//��ʽ��ȫѲ��
INSPECTOR Complete_Inspector_Static(void);
//�����ԾѲ��(Ѳ��Ա���̸���Ϊһ���ڵ�,�����ظ�ֵ)
INSPECTOR Active_Inspector_Forword(void);
//�����ԾѲ��(Ѳ��Ա���̻��˵���һ���ڵ�,���ظ�ֵ)
INSPECTOR Active_Inspector_Reverse(void);
//��ʽ��ԾѲ��
INSPECTOR Active_Inspector_Static(void);
/*************************
* ���ȼ�����
**************************/
//ָ��������ڵ����ȼ�����
int TOR_Priority_Set(TASK_ORBIT_RING* TOR, unsigned char Priority);
//�鿴ָ��������ڵ�����ȼ�
int TOR_Priority_Get(TASK_ORBIT_RING* TOR);

/*************************
* ����״̬���û���
**************************/
//����״̬
void TOR_Status_Set(TACTICS* tactics, TASK_STATUS status);
//��ȡ״̬
TASK_STATUS TOR_Status_Get(TACTICS* tactics);

/*************************
* ��ϣ�����
**************************/
//��ϣ���ʼ��
void Hash_Init(void);
//��ϣ�������(���ɼ�ֵ��)
unsigned int HashFunction(char* Task_Name);
//��ϣ��������
int HashTable_Insert(TASK_ORBIT_RING* TOR);
//��ϣ��ɾ������
int HashTable_Delete(char* Task_Name);
//��ϣ���ѯ����
TASK_ORBIT_RING* HashTable_Find(char* Task_Name);

/*************************
* ���ȼ�����������
**************************/
int SPC_Init(void);
//��ȡSPC_Head��ֵ
SCHEDULING_PRIORITY_CHAIN* Get_SPC_Head(void);
//��ȡSPC_Detector��ֵ
SCHEDULING_PRIORITY_CHAIN* Get_SPC_Detector(void);
//��ȡָ�����ȼ��ĵ������ڵ�
SCHEDULING_PRIORITY_CHAIN* Get_Designated_SPC(unsigned char Priority);
//��ѯ�Ƿ����ָ�����ȼ��ĵ������ڵ�
int SPC_Verify(unsigned char Priority);

/*************************
* ������������
**************************/
//����ָ��TOR
int TOR_Enable(TASK_ORBIT_RING* TOR);
//����ָ��TOR
int TOR_Disable(TASK_ORBIT_RING* TOR);
//�������ȼ�������Ѿ����õ�TOR
TASK_ORBIT_RING* TOR_Lowest_Enable_Find(void);
//�������ȼ�������Ѿ������Ҵ���ָ��״̬��TOR
TASK_ORBIT_RING* TOR_Lowest_Enable_Designated_Status_Find(TASK_STATUS status);


////�����ں˳�ʼ�� ����SPC��ʼ�� Hash��ʼ�� Ѳ��Ա��ʼ��
//#define Task_Core_Init()        SPC_Init();\
//                                Hash_Init(); \
//                                Inspector_Init()



//��������
void Task_Idle_Init(void);
void Task_Idle(void* args);


//int Task_Create(TASK_FUNCTION taskFunction, \
//    void* Args, \
//    unsigned char Priority, \
//    unsigned char* icon, \
//    char* Task_name, \
//    StackDepth size);



/*************************
* ����ͬ��
**************************/
//////////���ݽṹ����
/*
*	���ȴ����С�
*/
typedef struct Task_Wait_Tracker Task_Wait_Tracker;

struct Task_Wait_Tracker {
	TASK_ORBIT_RING* TOR; //�����������ڶ�λҪ��������
	unsigned int systick_time;//��¼��ʱ��ʱ�䣬ÿһ��ʱ��Ƭ�л��󣬾������Լ�
    //Mutex_t* mutex;//�������ĵ�ַ
	void* Synchronization_Primitives;//ͬ��ԭ����������ź������������������������������֤�Ƿ��Ŀ�������ͬһ������
    unsigned char block_type;//�������������ͣ�0Ϊ����������1Ϊ��ʱ������
	unsigned char enable_flag;//��ǰ�ĵȴ��ڵ��Ƿ���Ч��1Ϊ��Ч��0Ϊ��Ч��������Ч����Ч���ж���Ӧ�Զ�ʱ����ʱ���ֹ����������ɾ���ýڵ�Ļ���ʱ�䡾Ҳ����������ǵ�ǰ�ڵ��Ƿ����ɾ���ˡ���
	Task_Wait_Tracker* next;//��¼��һ���ṹ���λ��
};

/*
*	����������
*/
struct Mutex_t{
	TASK_ORBIT_RING* Possessor;//��������������
	unsigned char Status;//����״̬��0Ϊ������1Ϊ������
	unsigned char Initialization_flag;//�Ѿ���ʼ���ı�־��0Ϊ�Ѿ���ʼ��������0��ʾû�г�ʼ������Ϊ��0�ĸ���ҪԶ����0�ĸ��ʡ���
};


/*
*	�������ź�����
*/
//�����ź����ṹ��
struct Semaphore_t{
	unsigned int Max_Count;//���ֵ
	unsigned int Current_Count;//��ǰֵ
	unsigned char Initialization_flag;//��ʼ����־(0Ϊ��ʼ������0Ϊδ��ʼ��)
};



//ͬ��ԭ�����
void Synchronization_Primitives_Update(void);




#endif
