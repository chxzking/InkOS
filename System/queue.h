#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "inkos.h"
#include "heap_solution_1.h"

#define ONLY_ALLOWED_TO_SEND		2		//仅允许发送消息
#define ONLY_ALLOWED_RECEPTION		1		//仅允许接收消息

#define QUEUE_ENABLE 	1			//队列启用
#define QUEUE_DISABLE	0			//队列不启用

typedef struct Queue_Node Queue_Node;

struct Queue_Node{//队列的节点
	unsigned char Count;//相同数据数量
	void* Data;	//传输的数据
	Queue_Node* next;//下一个队列节点
};


struct Queue_t{//队列的头
	unsigned int Queue_Max_Count;//队列的最大节点数
	unsigned int Current_Count;//队列当前消息数量
	unsigned int Size;//数据的长度
	unsigned char SR_Flag;//收发权限标志，00第一位为发送权限，第二位为接收权限
	unsigned char enable_flag;//队列启用标志
	TASK_ORBIT_RING* Sender;//发送者
	TASK_ORBIT_RING* Receiver;//接收者
	Queue_Node* Read_Head;//读头，用于读取队列消息
	Queue_Node* Queue_Tail;//用于记录队列尾部，（作用是方便合并消息，减少存储）
};




#endif
