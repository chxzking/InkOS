#include "heap_solution_1.h"

#if HEAP_SCHEME == HEAP_SOLUTION_1                   //条件编译判断

/*内存分配器声明*/
static void* hj_memory_allocator(const DYNAMIC_MEMORY_TYPE __size);
/***************
 * 内存池操作
****************/
/*************************************
 * 【内存池——任务堆】
 * 专用于任务信息存储以及任务堆栈存储
 * ----------------------------------
 * [内存操作]
 * ----------------------------------
 * [位操作]
 *
**************************************/
//内存池
/***************
 * 内存池函数
****************/
static char TASK_MEMORY_POOL[TASK_POOL_SIZE] = { 0 };/*定义内存池,并且初始化为0，声明为静态变量,避免被其他的函数进行访问*/
/*每个字节的每个位代表该空间是否被使用，
 *如果被使用则置1
 *初始化为0，声明为静态变量,避免被其他的函数进行访问*/
static unsigned char TASK_POOL_USAGE_TAG[TASK_POOL_SIZE / BYTE_SIZE] = { 0 };/*内存池使用标志*/
//-------------------------------------------------------------起始-----------------------------------------------
//在内存池申请一个指定大小的空间
void* hj_malloc(DYNAMIC_MEMORY_TYPE __size) {
    if (__size == 0 || __size > TASK_POOL_SIZE)
        return NULL;
    return hj_memory_allocator(__size);
}
//对已申请的空间进行重新分配，根据参数判定扩大空间还是缩小空间
void* hj_realloc(void* __ptr, unsigned short __original__size, unsigned short __predistribute__size) {
    if (__predistribute__size <= 0 || __predistribute__size > TASK_POOL_SIZE)
        return NULL;
    if (!__ptr) {//传入空指针
        return NULL;
    }
    unsigned short size;//计算预期空间和原空间的差值
    unsigned short Absolute_Position;//计算出当前指针的绝对位置

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

        //当前指针往后是否能满足空间需求    
        char Spatial_Marker_Bit = 0;//判断标志
        for (int i = 0; i < size; i++) {
            Spatial_Marker_Bit = Bit_Read(TASK_POOL_USAGE_TAG, TASK_POOL_SIZE / BYTE_SIZE, Absolute_Position + i);
            if (Spatial_Marker_Bit)
                break;
        }

        if (!Spatial_Marker_Bit) {//检测到该数组后面存在足够的空间
            for (unsigned short i = 0; i < size; i++) {
                Bit_Modify(TASK_POOL_USAGE_TAG, TASK_POOL_SIZE / BYTE_SIZE, Absolute_Position + i, 1);//标志位置1 
            }
            return __ptr;
        }

        //没有足够空间，将将分配新空间
        Absolute_Position = (char*)__ptr - &TASK_MEMORY_POOL[0];
        void* New_Space_ptr = hj_memory_allocator(__predistribute__size);
        if (!New_Space_ptr)
            return NULL;//空间分配失败

        //将新空间赋值
        for (unsigned short i = 0; i < __original__size; i++) {
            *((char*)New_Space_ptr + i) = *((char*)__ptr + i);
            Bit_Modify(TASK_POOL_USAGE_TAG, TASK_POOL_SIZE / BYTE_SIZE, Absolute_Position + i, 0);
        }
        return New_Space_ptr;
    }
}
/*销毁从内存池申请的空间，同时将指针置空
 *【参数】
 * __ptr  -->动态空间地址
 * __size -->空间大小
 */
int hj_free(void* __ptr, DYNAMIC_MEMORY_TYPE __size) {
    if (!__ptr)//检测是否为空
        return -1;
    unsigned short Absolute_Position = (char*)__ptr - &TASK_MEMORY_POOL[0];//获取指针的绝对路径

    for (unsigned short i = 0; i < __size; i++) {
        if (Bit_Modify(TASK_POOL_USAGE_TAG, TASK_POOL_SIZE / BYTE_SIZE, Absolute_Position + i, 0) == -1)
            return -1;
    }
	
    __ptr = NULL;
    return 0;
}
/*内存分配器，是所有空间分配函数的基础函数
 *从内存池获取一片连续的空间
 *为了防止外部调用函数该函数，所以设置为静态函数
 *当内存获取成功后，返回内存区域的地址
 *如果内存获取失败则返回空指针
 *【参数】申请的内存大小
 */
static void* hj_memory_allocator(const DYNAMIC_MEMORY_TYPE __size) {
    void* memory_ptr = NULL;//内存指针
    DYNAMIC_MEMORY_TYPE count = 0;//统计当前已经分配的内存的数量

    //遍历内存池表，寻找空位置
    for (unsigned short i = 0; i < TASK_POOL_SIZE; i++) {
        signed char temp = Bit_Read(TASK_POOL_USAGE_TAG, TASK_POOL_SIZE / BYTE_SIZE, i);//获取每一位的信息
        if (temp == -1) {//空间分配失败
            return 0;
        }
        else if (temp == 1) {
            count = 0;//重新计算空位
            memory_ptr = 0;//指针置空
            if (i + count < TASK_POOL_SIZE)
                i += count;
        }
        else if (temp == 0) {
            if (count == 0)//首次发现空位，
                memory_ptr = &TASK_MEMORY_POOL[i];//获取该空位的指针
            count++;
        }

        if (count == __size) {//空间分配完成
            for (unsigned short j = 0; j < __size; j++) {
                //将内存池表置1
                Bit_Modify(TASK_POOL_USAGE_TAG, TASK_POOL_SIZE / BYTE_SIZE, i + 1 - __size + j, 1);

            }
            return memory_ptr;
        }
    }
    return NULL;//空间分配失败
}
/*统计内存池当前的剩余量*/
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

//-------------------------------------------------------------结束-----------------------------------------------



/*****************************************
*		         【用户堆】
******************************************
* 用于分配给用户、互斥锁等待链表、通信队列等
*******************************************/
/***************
 * 内存池函数
****************/
static char USER_MEMORY_POOL[USER_POOL_SIZE] = { 0 };/*定义用户内存池,并且初始化为0，声明为静态变量,避免被其他的函数进行访问*/
/*每个字节的每个位代表该空间是否被使用，
 *如果被使用则置1
 *初始化为0，声明为静态变量,避免被其他的函数进行访问*/
static unsigned char USER_POOL_USAGE_TAG[USER_POOL_SIZE / BYTE_SIZE] = { 0 };/*用户内存池使用标志*/

/*用户内存分配器声明*/
static void* Ink_memory_allocator(const DYNAMIC_MEMORY_TYPE __size);
//在内存池申请一个指定大小的空间
void* Ink_malloc(DYNAMIC_MEMORY_TYPE __size) {
    if (__size == 0 || __size > USER_POOL_SIZE)
        return NULL;
    return Ink_memory_allocator(__size);
}
//对已申请的空间进行重新分配，根据参数判定扩大空间还是缩小空间
void* Ink_realloc(void* __ptr, unsigned short __original__size, unsigned short __predistribute__size) {
    if (__predistribute__size <= 0 || __predistribute__size > USER_POOL_SIZE)
        return NULL;
    if (!__ptr) {//传入空指针
        return NULL;
    }
    unsigned short size;//计算预期空间和原空间的差值
    unsigned short Absolute_Position;//计算出当前指针的绝对位置

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

        //当前指针往后是否能满足空间需求    
        char Spatial_Marker_Bit = 0;//判断标志
        for (int i = 0; i < size; i++) {
            Spatial_Marker_Bit = Bit_Read(USER_POOL_USAGE_TAG, USER_POOL_SIZE / BYTE_SIZE, Absolute_Position + i);
            if (Spatial_Marker_Bit)
                break;
        }

        if (!Spatial_Marker_Bit) {//检测到该数组后面存在足够的空间
            for (unsigned short i = 0; i < size; i++) {
                Bit_Modify(USER_POOL_USAGE_TAG, USER_POOL_SIZE / BYTE_SIZE, Absolute_Position + i, 1);//标志位置1 
            }
            return __ptr;
        }

        //没有足够空间，将将分配新空间
        Absolute_Position = (char*)__ptr - &USER_MEMORY_POOL[0];
        void* New_Space_ptr = hj_memory_allocator(__predistribute__size);
        if (!New_Space_ptr)
            return NULL;//空间分配失败

        //将新空间赋值
        for (unsigned short i = 0; i < __original__size; i++) {
            *((char*)New_Space_ptr + i) = *((char*)__ptr + i);
            Bit_Modify(USER_POOL_USAGE_TAG, USER_POOL_SIZE / BYTE_SIZE, Absolute_Position + i, 0);
        }
        return New_Space_ptr;
    }
}
//销毁空间
int Ink_free(void* __ptr, DYNAMIC_MEMORY_TYPE __size) {
    if (!__ptr)//检测是否为空
        return -1;
    unsigned short Absolute_Position = (char*)__ptr - &USER_MEMORY_POOL[0];//获取指针的绝对路径

    for (unsigned short i = 0; i < __size; i++) {
        if (Bit_Modify(USER_POOL_USAGE_TAG, USER_POOL_SIZE / BYTE_SIZE, Absolute_Position + i, 0) == -1)
            return -1;
    }
    __ptr = NULL;
    return 0;
}
/*内存分配器，是所有空间分配函数的基础函数
 *从内存池获取一片连续的空间
 *为了防止外部调用函数该函数，所以设置为静态函数
 *当内存获取成功后，返回内存区域的地址
 *如果内存获取失败则返回空指针
 *【参数】申请的内存大小
 */
static void* Ink_memory_allocator(const DYNAMIC_MEMORY_TYPE __size) {
    void* memory_ptr = NULL;//内存指针
    DYNAMIC_MEMORY_TYPE count = 0;//统计当前已经分配的内存的数量

    //遍历内存池表，寻找空位置
    for (unsigned short i = 0; i < USER_POOL_SIZE; i++) {
        signed char temp = Bit_Read(USER_POOL_USAGE_TAG, USER_POOL_SIZE / BYTE_SIZE, i);//获取每一位的信息
        if (temp == -1) {//空间分配失败
            return 0;
        }
        else if (temp == 1) {
            count = 0;//重新计算空位
            memory_ptr = 0;//指针置空
            if (i + count < USER_POOL_SIZE)
                i += count;
        }
        else if (temp == 0) {
            if (count == 0)//首次发现空位，
                memory_ptr = &USER_MEMORY_POOL[i];//获取该空位的指针
            count++;
        }

        if (count == __size) {//空间分配完成
            for (unsigned short j = 0; j < __size; j++) {
                //将内存池表置1
                Bit_Modify(USER_POOL_USAGE_TAG, USER_POOL_SIZE / BYTE_SIZE, i + 1 - __size + j, 1);

            }
            return memory_ptr;
        }
    }
    return NULL;//空间分配失败
}

//用户堆内存剩余量统计
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
