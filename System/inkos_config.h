#pragma once
#ifndef INKOS_CONFIG_H
#define INKOS_CONFIG_H

//内存池相关定义
#define TASK_POOL_SIZE 				4096 						//定义任务内存池的大小
#define USER_POOL_SIZE				1024						//定义用户内存池大小
#define BYTE_SIZE				    8 							//一个字节的长度

//Task相关定义
#define TASK_NAME_SIZE 10                                       //定义任务名的长度
#define STACK_DETECTION_VALUE 0xffffffff                        //栈底设置一个特定值
#define HASH_TABLE_SIZE 16                                      //设定哈希表的大小（通常为2的幂）

#define STACK_DEPTH_TYPE unsigned short                         //栈的深度重定义的基本数据类型
#define STACK_T_TYPE unsigned int                               //栈类型重定义的基础数据类型

//空闲任务
#define TASK_IDLE_STACK_SIZE 32									//空闲任务堆栈大小设置
//-----------------------------------------特殊优先级策略 起始分割线----------------------------------------
//特殊优先级方案
#define SPECIAL_PRIORITY_ENABLE		1							//关闭特殊优先级
#define SPECIAL_PRIORITY_DISABLE	0							//开启特殊优先级
//特殊优先级配置
#define SPECIAL_PRIORITY_CONFIG		SPECIAL_PRIORITY_DISABLE	//默认不开启
//-----------------------------------------特殊优先级策略 结束分割线----------------------------------------


//-----------------------------------------堆区使用定义 起始分割线----------------------------------------
//堆区解决方案列表
#define HEAP_SOLUTION_1 1                                       //堆区解决方案1  
//...添加其他堆区方案

//配置堆区解决方案
#define HEAP_SCHEME HEAP_SOLUTION_1                             //宏定义选择解决方案（默认选择方案1）
//-----------------------------------------堆区使用定义 结束分割线----------------------------------------



//-----------------------------------------任务加载优化 起始分割线----------------------------------------
//任务加载优化方案
#define TASK_LOAD_SOLUTION_1	1								//方案1，当内存不够时，拒绝新任务的加载申请
#define TASK_LOAD_SOLUTION_2	2								//方案2，当内存不够时，如果新任务优先级在活跃状态任务中同级或者更高，则释放优先级低的任务.否则拒绝任务加载的申请
#define TASK_LOAD_SOLUTION_3	3								//方案3，当内存不够时，如果新任务优先级非最低，则释放最低优先级其中一个任务，添加新任务
//...添加其他任务加载解决方案(有设计保存被释放的任务进入flash的想法)

//配置任务加载解决方案
#define TASK_LOAD_SOLUTION			 TASK_LOAD_SOLUTION_2		//宏定义选择方案（默认选择方案2）
//-----------------------------------------任务加载优化 结束分割线----------------------------------------

//-----------------------------------------通信队列策略 开始分割线----------------------------------------
//通信队列策略方案
#define Synchronous_Communication_Mode		1					//策略1：同步通信模式：接收方与发送方必须严格使用同一种Message_Type，否则引发死锁
#define Asynchronous_Communication_Mode		2					//策略2：异步通信模式：接收方与发送方不需要使用同一种Message_Type，但是会有轻微的性能损耗
//配置策略
#define MESSAGE_MODE Synchronous_Communication_Mode				//默认使用——策略1
//-----------------------------------------通信队列策略 结束分割线----------------------------------------

//时间片配置
//默认全速模式
#define SYSTICK_CLK_FREQ	72		//时间片频率（MHZ）
#define SYSTICK_INTERVAL_MS 10		//时间片长度（毫秒）
#define SYSCLK_DIV 1				//分频（默认不分）
#define SYSTICK_LOAD (SYSTICK_INTERVAL_MS*1000*(SYSTICK_CLK_FREQ/SYSCLK_DIV) - 1);



#endif
