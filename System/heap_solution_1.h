#pragma once

#include "inkos_config.h"

#if HEAP_SCHEME == HEAP_SOLUTION_1          //���������ж�

#ifndef _HEAP_SOLUTION_1_H_
#define _HEAP_SOLUTION_1_H_
//����

#include "inkos.h"
#include "bit_operate.h"
/*************************************
 * ���ڴ�ء�
 *
 * ----------------------------------
 * [�ڴ����]
 * ----------------------------------
 * [λ����]
 *
**************************************/


///*��̬�ڴ����*/
//typedef unsigned short DYNAMIC_MEMORY_TYPE; 					/*����һ��ͳһ�Ķ�̬�ڴ��������*/

/*���ڴ������һƬ�ռ�
 *����ɹ���᷵�ؿռ��ָ��
 *����ʧ���򷵻�NULLָ��
 *������������ռ�Ĵ�С
 */
void* hj_malloc(DYNAMIC_MEMORY_TYPE __size);
/*���ڴ������һƬ�ռ�
 *����ɹ���᷵�ؿռ��ָ��
 *����ʧ���򷵻�NULLָ��
 *������������ռ�Ĵ�С
 */
void* hj_realloc(void* __ptr, unsigned short __original__size, unsigned short __predistribute__size);
/*���ٴ��ڴ������Ŀռ䣬ͬʱ��ָ���ÿ�
 *��������
 * __ptr  -->��̬�ռ��ַ
 * __size -->�ռ��С
 */
int hj_free(void* __ptr, unsigned short __size);

//-------------------------------------------------------------����-----------------------------------------------

#endif

#endif


