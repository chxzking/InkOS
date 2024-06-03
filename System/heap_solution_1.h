#pragma once

#include "inkos_config.h"

#if HEAP_SCHEME == HEAP_SOLUTION_1          //条件编译判断

#ifndef _HEAP_SOLUTION_1_H_
#define _HEAP_SOLUTION_1_H_
//代码

#include "inkos.h"
#include "bit_operate.h"
/*************************************
 * 【内存池】
 *
 * ----------------------------------
 * [内存操作]
 * ----------------------------------
 * [位操作]
 *
**************************************/


///*动态内存分配*/
//typedef unsigned short DYNAMIC_MEMORY_TYPE; 					/*定义一个统一的动态内存分配类型*/

/*在内存池申请一片空间
 *申请成功后会返回空间的指针
 *申请失败则返回NULL指针
 *【参数】申请空间的大小
 */
void* hj_malloc(DYNAMIC_MEMORY_TYPE __size);
/*在内存池申请一片空间
 *申请成功后会返回空间的指针
 *申请失败则返回NULL指针
 *【参数】申请空间的大小
 */
void* hj_realloc(void* __ptr, unsigned short __original__size, unsigned short __predistribute__size);
/*销毁从内存池申请的空间，同时将指针置空
 *【参数】
 * __ptr  -->动态空间地址
 * __size -->空间大小
 */
int hj_free(void* __ptr, unsigned short __size);

//-------------------------------------------------------------结束-----------------------------------------------

#endif

#endif


