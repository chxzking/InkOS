#include "heap_solution_1.h"

#if HEAP_SCHEME == HEAP_SOLUTION_1                   //���������ж�

/*�ڴ����������*/
static void* hj_memory_allocator(const DYNAMIC_MEMORY_TYPE __size);
/***************
 * �ڴ�ز���
****************/
/*************************************
 * ���ڴ�ء�������ѡ�
 * ר����������Ϣ�洢�Լ������ջ�洢
 * ----------------------------------
 * [�ڴ����]
 * ----------------------------------
 * [λ����]
 *
**************************************/
//�ڴ��
/***************
 * �ڴ�غ���
****************/
static char TASK_MEMORY_POOL[TASK_POOL_SIZE] = { 0 };/*�����ڴ��,���ҳ�ʼ��Ϊ0������Ϊ��̬����,���ⱻ�����ĺ������з���*/
/*ÿ���ֽڵ�ÿ��λ����ÿռ��Ƿ�ʹ�ã�
 *�����ʹ������1
 *��ʼ��Ϊ0������Ϊ��̬����,���ⱻ�����ĺ������з���*/
static unsigned char TASK_POOL_USAGE_TAG[TASK_POOL_SIZE / BYTE_SIZE] = { 0 };/*�ڴ��ʹ�ñ�־*/
//-------------------------------------------------------------��ʼ-----------------------------------------------
//���ڴ������һ��ָ����С�Ŀռ�
void* hj_malloc(DYNAMIC_MEMORY_TYPE __size) {
    if (__size == 0 || __size > TASK_POOL_SIZE)
        return NULL;
    return hj_memory_allocator(__size);
}
//��������Ŀռ�������·��䣬���ݲ����ж�����ռ仹����С�ռ�
void* hj_realloc(void* __ptr, unsigned short __original__size, unsigned short __predistribute__size) {
    if (__predistribute__size <= 0 || __predistribute__size > TASK_POOL_SIZE)
        return NULL;
    if (!__ptr) {//�����ָ��
        return NULL;
    }
    unsigned short size;//����Ԥ�ڿռ��ԭ�ռ�Ĳ�ֵ
    unsigned short Absolute_Position;//�������ǰָ��ľ���λ��

    if (__original__size > __predistribute__size) {
        size = __original__size - __predistribute__size;
        Absolute_Position = ((char*)__ptr + __predistribute__size) - &TASK_MEMORY_POOL[0];
        for (unsigned short i = 0; i < size; i++) {
            Bit_Modify(TASK_POOL_USAGE_TAG, TASK_POOL_SIZE / BYTE_SIZE, Absolute_Position + i, 0);
        }
        return __ptr;
    }
    else {
        size = __predistribute__size - __original__size;
        Absolute_Position = ((char*)__ptr + __original__size) - &TASK_MEMORY_POOL[0];

        //��ǰָ�������Ƿ�������ռ�����    
        char Spatial_Marker_Bit = 0;//�жϱ�־
        for (int i = 0; i < size; i++) {
            Spatial_Marker_Bit = Bit_Read(TASK_POOL_USAGE_TAG, TASK_POOL_SIZE / BYTE_SIZE, Absolute_Position + i);
            if (Spatial_Marker_Bit)
                break;
        }

        if (!Spatial_Marker_Bit) {//��⵽�������������㹻�Ŀռ�
            for (unsigned short i = 0; i < size; i++) {
                Bit_Modify(TASK_POOL_USAGE_TAG, TASK_POOL_SIZE / BYTE_SIZE, Absolute_Position + i, 1);//��־λ��1 
            }
            return __ptr;
        }

        //û���㹻�ռ䣬���������¿ռ�
        Absolute_Position = (char*)__ptr - &TASK_MEMORY_POOL[0];
        void* New_Space_ptr = hj_memory_allocator(__predistribute__size);
        if (!New_Space_ptr)
            return NULL;//�ռ����ʧ��

        //���¿ռ丳ֵ
        for (unsigned short i = 0; i < __original__size; i++) {
            *((char*)New_Space_ptr + i) = *((char*)__ptr + i);
            Bit_Modify(TASK_POOL_USAGE_TAG, TASK_POOL_SIZE / BYTE_SIZE, Absolute_Position + i, 0);
        }
        return New_Space_ptr;
    }
}
/*���ٴ��ڴ������Ŀռ䣬ͬʱ��ָ���ÿ�
 *��������
 * __ptr  -->��̬�ռ��ַ
 * __size -->�ռ��С
 */
int hj_free(void* __ptr, DYNAMIC_MEMORY_TYPE __size) {
    if (!__ptr)//����Ƿ�Ϊ��
        return -1;
    unsigned short Absolute_Position = (char*)__ptr - &TASK_MEMORY_POOL[0];//��ȡָ��ľ���·��

    for (unsigned short i = 0; i < __size; i++) {
        if (Bit_Modify(TASK_POOL_USAGE_TAG, TASK_POOL_SIZE / BYTE_SIZE, Absolute_Position + i, 0) == -1)
            return -1;
    }
	
    __ptr = NULL;
    return 0;
}
/*�ڴ�������������пռ���亯���Ļ�������
 *���ڴ�ػ�ȡһƬ�����Ŀռ�
 *Ϊ�˷�ֹ�ⲿ���ú����ú�������������Ϊ��̬����
 *���ڴ��ȡ�ɹ��󣬷����ڴ�����ĵ�ַ
 *����ڴ��ȡʧ���򷵻ؿ�ָ��
 *��������������ڴ��С
 */
static void* hj_memory_allocator(const DYNAMIC_MEMORY_TYPE __size) {
    void* memory_ptr = NULL;//�ڴ�ָ��
    DYNAMIC_MEMORY_TYPE count = 0;//ͳ�Ƶ�ǰ�Ѿ�������ڴ������

    //�����ڴ�ر�Ѱ�ҿ�λ��
    for (unsigned short i = 0; i < TASK_POOL_SIZE; i++) {
        signed char temp = Bit_Read(TASK_POOL_USAGE_TAG, TASK_POOL_SIZE / BYTE_SIZE, i);//��ȡÿһλ����Ϣ
        if (temp == -1) {//�ռ����ʧ��
            return 0;
        }
        else if (temp == 1) {
            count = 0;//���¼����λ
            memory_ptr = 0;//ָ���ÿ�
            if (i + count < TASK_POOL_SIZE)
                i += count;
        }
        else if (temp == 0) {
            if (count == 0)//�״η��ֿ�λ��
                memory_ptr = &TASK_MEMORY_POOL[i];//��ȡ�ÿ�λ��ָ��
            count++;
        }

        if (count == __size) {//�ռ�������
            for (unsigned short j = 0; j < __size; j++) {
                //���ڴ�ر���1
                Bit_Modify(TASK_POOL_USAGE_TAG, TASK_POOL_SIZE / BYTE_SIZE, i + 1 - __size + j, 1);

            }
            return memory_ptr;
        }
    }
    return NULL;//�ռ����ʧ��
}
/*ͳ���ڴ�ص�ǰ��ʣ����*/
int Task_Statistical_Free_Heap(void) {
    int count = 0;
    for (unsigned short i = 0; i < TASK_POOL_SIZE / BYTE_SIZE; i++) {
        for (unsigned char j = 0; j < 8; j++) {
            if (!Bit_Read_Within_Byte(&TASK_POOL_USAGE_TAG[i], j))
                count++;
        }
    }
    return count;
}

//-------------------------------------------------------------����-----------------------------------------------



/*****************************************
*		         ���û��ѡ�
******************************************
* ���ڷ�����û����������ȴ�����ͨ�Ŷ��е�
*******************************************/
/***************
 * �ڴ�غ���
****************/
static char USER_MEMORY_POOL[USER_POOL_SIZE] = { 0 };/*�����û��ڴ��,���ҳ�ʼ��Ϊ0������Ϊ��̬����,���ⱻ�����ĺ������з���*/
/*ÿ���ֽڵ�ÿ��λ����ÿռ��Ƿ�ʹ�ã�
 *�����ʹ������1
 *��ʼ��Ϊ0������Ϊ��̬����,���ⱻ�����ĺ������з���*/
static unsigned char USER_POOL_USAGE_TAG[USER_POOL_SIZE / BYTE_SIZE] = { 0 };/*�û��ڴ��ʹ�ñ�־*/

/*�û��ڴ����������*/
static void* Ink_memory_allocator(const DYNAMIC_MEMORY_TYPE __size);
//���ڴ������һ��ָ����С�Ŀռ�
void* Ink_malloc(DYNAMIC_MEMORY_TYPE __size) {
    if (__size == 0 || __size > USER_POOL_SIZE)
        return NULL;
    return Ink_memory_allocator(__size);
}
//��������Ŀռ�������·��䣬���ݲ����ж�����ռ仹����С�ռ�
void* Ink_realloc(void* __ptr, unsigned short __original__size, unsigned short __predistribute__size) {
    if (__predistribute__size <= 0 || __predistribute__size > USER_POOL_SIZE)
        return NULL;
    if (!__ptr) {//�����ָ��
        return NULL;
    }
    unsigned short size;//����Ԥ�ڿռ��ԭ�ռ�Ĳ�ֵ
    unsigned short Absolute_Position;//�������ǰָ��ľ���λ��

    if (__original__size > __predistribute__size) {
        size = __original__size - __predistribute__size;
        Absolute_Position = ((char*)__ptr + __predistribute__size) - &USER_MEMORY_POOL[0];
        for (unsigned short i = 0; i < size; i++) {
            Bit_Modify(USER_POOL_USAGE_TAG, USER_POOL_SIZE / BYTE_SIZE, Absolute_Position + i, 0);
        }
        return __ptr;
    }
    else {
        size = __predistribute__size - __original__size;
        Absolute_Position = ((char*)__ptr + __original__size) - &USER_MEMORY_POOL[0];

        //��ǰָ�������Ƿ�������ռ�����    
        char Spatial_Marker_Bit = 0;//�жϱ�־
        for (int i = 0; i < size; i++) {
            Spatial_Marker_Bit = Bit_Read(USER_POOL_USAGE_TAG, USER_POOL_SIZE / BYTE_SIZE, Absolute_Position + i);
            if (Spatial_Marker_Bit)
                break;
        }

        if (!Spatial_Marker_Bit) {//��⵽�������������㹻�Ŀռ�
            for (unsigned short i = 0; i < size; i++) {
                Bit_Modify(USER_POOL_USAGE_TAG, USER_POOL_SIZE / BYTE_SIZE, Absolute_Position + i, 1);//��־λ��1 
            }
            return __ptr;
        }

        //û���㹻�ռ䣬���������¿ռ�
        Absolute_Position = (char*)__ptr - &USER_MEMORY_POOL[0];
        void* New_Space_ptr = hj_memory_allocator(__predistribute__size);
        if (!New_Space_ptr)
            return NULL;//�ռ����ʧ��

        //���¿ռ丳ֵ
        for (unsigned short i = 0; i < __original__size; i++) {
            *((char*)New_Space_ptr + i) = *((char*)__ptr + i);
            Bit_Modify(USER_POOL_USAGE_TAG, USER_POOL_SIZE / BYTE_SIZE, Absolute_Position + i, 0);
        }
        return New_Space_ptr;
    }
}
//���ٿռ�
int Ink_free(void* __ptr, DYNAMIC_MEMORY_TYPE __size) {
    if (!__ptr)//����Ƿ�Ϊ��
        return -1;
    unsigned short Absolute_Position = (char*)__ptr - &USER_MEMORY_POOL[0];//��ȡָ��ľ���·��

    for (unsigned short i = 0; i < __size; i++) {
        if (Bit_Modify(USER_POOL_USAGE_TAG, USER_POOL_SIZE / BYTE_SIZE, Absolute_Position + i, 0) == -1)
            return -1;
    }
    __ptr = NULL;
    return 0;
}
/*�ڴ�������������пռ���亯���Ļ�������
 *���ڴ�ػ�ȡһƬ�����Ŀռ�
 *Ϊ�˷�ֹ�ⲿ���ú����ú�������������Ϊ��̬����
 *���ڴ��ȡ�ɹ��󣬷����ڴ�����ĵ�ַ
 *����ڴ��ȡʧ���򷵻ؿ�ָ��
 *��������������ڴ��С
 */
static void* Ink_memory_allocator(const DYNAMIC_MEMORY_TYPE __size) {
    void* memory_ptr = NULL;//�ڴ�ָ��
    DYNAMIC_MEMORY_TYPE count = 0;//ͳ�Ƶ�ǰ�Ѿ�������ڴ������

    //�����ڴ�ر�Ѱ�ҿ�λ��
    for (unsigned short i = 0; i < USER_POOL_SIZE; i++) {
        signed char temp = Bit_Read(USER_POOL_USAGE_TAG, USER_POOL_SIZE / BYTE_SIZE, i);//��ȡÿһλ����Ϣ
        if (temp == -1) {//�ռ����ʧ��
            return 0;
        }
        else if (temp == 1) {
            count = 0;//���¼����λ
            memory_ptr = 0;//ָ���ÿ�
            if (i + count < USER_POOL_SIZE)
                i += count;
        }
        else if (temp == 0) {
            if (count == 0)//�״η��ֿ�λ��
                memory_ptr = &USER_MEMORY_POOL[i];//��ȡ�ÿ�λ��ָ��
            count++;
        }

        if (count == __size) {//�ռ�������
            for (unsigned short j = 0; j < __size; j++) {
                //���ڴ�ر���1
                Bit_Modify(USER_POOL_USAGE_TAG, USER_POOL_SIZE / BYTE_SIZE, i + 1 - __size + j, 1);

            }
            return memory_ptr;
        }
    }
    return NULL;//�ռ����ʧ��
}

//�û����ڴ�ʣ����ͳ��
int User_Statistical_Free_Heap(void) {
    int count = 0;
    for (unsigned short i = 0; i < USER_POOL_SIZE / BYTE_SIZE; i++) {
        for (unsigned char j = 0; j < 8; j++) {
            if (!Bit_Read_Within_Byte(&USER_POOL_USAGE_TAG[i], j))
                count++;
        }
    }
    return count;
}
#endif
