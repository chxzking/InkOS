#pragma once
#ifndef _TASK_H_
#define _TASK_H_

#include "inkos.h"
#include "inkos_config.h"

#if HEAP_SCHEME == HEAP_SOLUTION_1  
#include "heap_solution_1.h"
#endif

#include "bit_operate.h"

//数据结构声明
typedef struct TaskControlBlock TaskControlBlock;				                //任务TCB模块
//typedef struct TASK_ORBIT_RING TASK_ORBIT_RING;				                    //定义任务轨道环
typedef struct HashNode HashNode;                                               //定义哈希节点数据结构
typedef struct HashTable HashTable;                                             //定义哈希表数据结构
//typedef struct SCHEDULING_PRIORITY_CHAIN SCHEDULING_PRIORITY_CHAIN;             //任务计划链
//typedef struct INSPECTOR INSPECTOR;                                             //巡查员

typedef enum TASK_STATUS TASK_STATUS;                                           //任务状态枚举声明

//数据类型重定义
//typedef void (*TASK_FUNCTION)(void*);							                //任务函数
typedef  unsigned char TACTICS;                                                 //策略结构
//typedef STACK_DEPTH_TYPE StackDepth;                                            //栈的深度重定义
typedef STACK_T_TYPE stack_t;                                                   //栈类型重定义

//定义SPC节点数据类型
#define SPC_TYPE_HEADER     0                                                   //头节点类型
#define SPC_TYPE_GENERAL    1                                                   //普通节点类型

#define PRIORITY_MASK       0x0f                                                //优先级基准掩码

//设置指定TOR的状态
#define STATUS_MASK         0xc0                                                //状态掩码
#define STATUS_BASIS_CODE   0x40                                                //状态基准修改码
#define TOR_ENABLE_FLAG     1                                                   //TOR启用
#define TOR_DISABLE_FLAG    0                                                   //TOR不启用

//设置新任务标志
#define NEW_TASK_AVAILABLE      1                                                   //有新任务加入标志
#define NEW_TASK_NOT_AVAILABLE  0                                                   //没有新任务加入标志

 //任务状态

//数据结构定义
/*TCB任务控制块定义*/
struct TaskControlBlock {
    stack_t* stack_top;                                                         //栈顶
    stack_t* stack_bottom;                                                      //栈底
    StackDepth stack_size;                                                      //堆栈大小
    TASK_FUNCTION taskFunction;                                                 //任务函数
    void* Args;                                                                 //参数
};
/*TOR任务轨道环定义*/
struct TASK_ORBIT_RING {
    //任务底层
    TaskControlBlock TCB;                                                       //TCB
    TASK_ORBIT_RING* self_handle;                                               //自身句柄
    SCHEDULING_PRIORITY_CHAIN* self_SPC;                                        //当前挂载的SPC
    TACTICS tactics;                                                            //策略（定义序号从高位往低位递增，[0]-[1]任务状态，[2]-[3]保留，[4]-[7]优先级）
    unsigned char Task_Enable_Flag;                                             //任务启用标志
    //任务上层
    unsigned char* icon;                                                        //任务logo的字模  
    //队列指针区
    TASK_ORBIT_RING* next;                                                      //后一个任务的指针
    TASK_ORBIT_RING* previous;                                                  //前一个任务的指针
    //任务名称区
    char Task_name[TASK_NAME_SIZE + 1];                                         //任务名称
};
/*SPC优先级任务调度链定义*/
struct SCHEDULING_PRIORITY_CHAIN {
    TASK_ORBIT_RING* Task_Ring;                                                 //任务环挂载点（头节点置NULL）
    unsigned char Ring_Priority;                                                //环的优先级 （头结点置0）
    unsigned char Ring_Task_Amount;                                             //环中注册的任务的总数（头节点统计SPC的数量）
    unsigned char Ring_Active_Amount;                                           //环中的任务处于活跃状态（就绪和运行态）的总数  (头结点有打算实现统计非空SPC的数量，但是还有实现代码) 
    unsigned char SPC_Type;                                                     //优先级调度链节点类型（1为普通类型，0为头结点）
    TASK_ORBIT_RING* TOR_Leader;                                                //TOR环中的环首标记
    SCHEDULING_PRIORITY_CHAIN* next;                                            //(头结点中，这个存储SPC第一个节点的地址)
    SCHEDULING_PRIORITY_CHAIN* previous;                                        //(头结点中，这个用来存储SPC最后一个节点的地址)
};
//巡查员定义
struct INSPECTOR {
    SCHEDULING_PRIORITY_CHAIN* SPC;                                             //SPC
    TASK_ORBIT_RING* TOR;                                                       //TOR
};
/*任务类型枚举定义*/
enum TASK_STATUS {
    TASK_OBSTRUCT,//阻塞状态
    TASK_READY,//就绪状态
    TASK_RUNTIME,//运行状态
    TASK_ERROR//错误值
};
/*哈希表节点定义*/
struct HashNode {
    TASK_ORBIT_RING* TOR;  // 指向TOR的指针,包含了存储任务名称
    HashNode* next;  // 处理冲突，使用链表法
};
/*哈希表定义*/
struct HashTable {
    HashNode* array[HASH_TABLE_SIZE];  // 哈希表数组
    int size;  // 哈希表实际存储的元素数量
};

///*栈溢出信号类型*/
//enum Stack_Overflow_t{
//	STACK_OVERFLOW = -1,//栈溢出
//	STACK_SAFETY = 0,	//栈安全
//	STACK_FULL = 1		//栈满（处于溢出边界）
//};

/*************************
* 巡查员
**************************/
//初始化巡查
void Inspector_Init(void);
//正向完全巡查(巡查员立刻更新为一个节点,并返回该值)
INSPECTOR Complete_Inspector_Forword(void);
//逆向完全巡查(巡查员立刻回退到上一个节点,返回该值)
INSPECTOR Complete_Inspector_Reverse(void);
//静式完全巡查
INSPECTOR Complete_Inspector_Static(void);
//正向活跃巡查(巡查员立刻更新为一个节点,并返回该值)
INSPECTOR Active_Inspector_Forword(void);
//逆向活跃巡查(巡查员立刻回退到上一个节点,返回该值)
INSPECTOR Active_Inspector_Reverse(void);
//静式活跃巡查
INSPECTOR Active_Inspector_Static(void);
/*************************
* 优先级基础
**************************/
//指定轨道环节点优先级设置
int TOR_Priority_Set(TASK_ORBIT_RING* TOR, unsigned char Priority);
//查看指定轨道环节点的优先级
int TOR_Priority_Get(TASK_ORBIT_RING* TOR);

/*************************
* 任务状态设置基础
**************************/
//设置状态
void TOR_Status_Set(TACTICS* tactics, TASK_STATUS status);
//获取状态
TASK_STATUS TOR_Status_Get(TACTICS* tactics);

/*************************
* 哈希表操作
**************************/
//哈希表初始化
void Hash_Init(void);
//哈希函数设计(生成键值对)
unsigned int HashFunction(char* Task_Name);
//哈希表插入操作
int HashTable_Insert(TASK_ORBIT_RING* TOR);
//哈希表删除操作
int HashTable_Delete(char* Task_Name);
//哈希表查询操作
TASK_ORBIT_RING* HashTable_Find(char* Task_Name);

/*************************
* 优先级调度链操作
**************************/
int SPC_Init(void);
//获取SPC_Head的值
SCHEDULING_PRIORITY_CHAIN* Get_SPC_Head(void);
//获取SPC_Detector的值
SCHEDULING_PRIORITY_CHAIN* Get_SPC_Detector(void);
//获取指定优先级的调度链节点
SCHEDULING_PRIORITY_CHAIN* Get_Designated_SPC(unsigned char Priority);
//查询是否存在指定优先级的调度链节点
int SPC_Verify(unsigned char Priority);

/*************************
* 任务轨道环操作
**************************/
//启用指定TOR
int TOR_Enable(TASK_ORBIT_RING* TOR);
//禁用指定TOR
int TOR_Disable(TASK_ORBIT_RING* TOR);
//返回优先级最低且已经启用的TOR
TASK_ORBIT_RING* TOR_Lowest_Enable_Find(void);
//返回优先级最低且已经启用且处于指定状态的TOR
TASK_ORBIT_RING* TOR_Lowest_Enable_Designated_Status_Find(TASK_STATUS status);


////任务内核初始化 ——SPC初始化 Hash初始化 巡查员初始化
//#define Task_Core_Init()        SPC_Init();\
//                                Hash_Init(); \
//                                Inspector_Init()



//空闲任务
void Task_Idle_Init(void);
void Task_Idle(void* args);


//int Task_Create(TASK_FUNCTION taskFunction, \
//    void* Args, \
//    unsigned char Priority, \
//    unsigned char* icon, \
//    char* Task_name, \
//    StackDepth size);



/*************************
* 进程同步
**************************/
//////////数据结构定义
/*
*	【等待队列】
*/
typedef struct Task_Wait_Tracker Task_Wait_Tracker;

struct Task_Wait_Tracker {
	TASK_ORBIT_RING* TOR; //任务句柄，用于定位要唤醒任务
	unsigned int systick_time;//记录延时的时间，每一次时间片切换后，就立刻自减
    //Mutex_t* mutex;//互斥锁的地址
	void* Synchronization_Primitives;//同步原语（包括计数信号量、互斥锁）【这个作用是用于验证是否和目标的锁是同一个锁】
    unsigned char block_type;//阻塞互斥锁类型（0为永久阻塞，1为定时阻塞）
	unsigned char enable_flag;//当前的等待节点是否还有效（1为有效，0为无效，设置有效和无效的判断是应对定时阻塞时间截止后，留给任务删除该节点的缓冲时间【也就是用来标记当前节点是否可以删除了】）
	Task_Wait_Tracker* next;//记录下一个结构体的位置
};

/*
*	【互斥锁】
*/
struct Mutex_t{
	TASK_ORBIT_RING* Possessor;//互斥锁的所有者
	unsigned char Status;//锁的状态（0为解锁，1为上锁）
	unsigned char Initialization_flag;//已经初始化的标志（0为已经初始化过，非0表示没有初始化【因为非0的概率要远高于0的概率】）
};


/*
*	【计数信号量】
*/
//计数信号量结构体
struct Semaphore_t{
	unsigned int Max_Count;//最大值
	unsigned int Current_Count;//当前值
	unsigned char Initialization_flag;//初始化标志(0为初始化，非0为未初始化)
};



//同步原语更新
void Synchronization_Primitives_Update(void);




#endif
