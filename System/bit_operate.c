#include "bit_operate.h"


/***************
 * λ����
****************/
//-------------------------------------------------------------��ʼ-----------------------------------------------
//����������ָ��,��ţ�Ŀ��ֵ������������۽���ĳһ���ֽڣ��Ը��ֽ��е�λ������0������1����������������Ĭ�Ͻ�����ķ���ֵ��Ϊ1����ֵ��Ϊ0���������ִ��ʧ�ܻ᷵��-1
/*�Ե����ֽڵ�8��λΪ���ģ��޸�λ��ֵ
 *��һ���ֽڳ���Ϊ8�����ȵ����飬
 *���дӸ�λ����λ��ѯ�����λ����Ϊ�±�[0]�����λ����Ϊ�±�[7]
 *��������
 * __format         -->Ŀ���ֽڵĵ�ַ
 * __serial__number -->Ŀ��λ���±����
 * __binary_number  -->�޸ĺ��ֵ����ֵ��Ϊ0ʱ�����ᱻĬ���޸�Ϊ1����ֵΪ0ʱ�����޸�Ϊ0
 * �����ɹ�����0�����򷵻�-1��
 */
int Bit_Modify_Within_Byte(unsigned char* __format, unsigned char __serial__number, unsigned char __binary_number) {
    if (__serial__number >= BYTE_SIZE)//Խ�����
        return -1;
    if (__binary_number)
        (*__format) |= (BIT_MASK >> __serial__number);
    else
        (*__format) &= ~(BIT_MASK >> __serial__number);
    return 0;
}
/*�Ե����ֽڵ�8��λΪ���ģ���ȡָ��λ�õ�ֵ
 *��һ���ֽڳ���Ϊ8�����ȵ����飬
 *���дӸ�λ����λ��ѯ�����λ����Ϊ�±�[0]�����λ����Ϊ�±�[7]
 *��������
 * __format         -->Ŀ���ֽڵĵ�ַ
 * __serial__number -->Ŀ��λ���±����
 * �����ɹ����ض�ȡ��ֵ�����򷵻�-1��
 */
signed char Bit_Read_Within_Byte(const unsigned char* __format, unsigned char __serial__number) {
    if (__serial__number >= BYTE_SIZE)//Խ�����
        return -1;
    unsigned char temp = ((*__format) & (BIT_MASK >> __serial__number)) ? 1 : 0;
    return temp;
}
/*��������������Ϊ���ģ���ȡָ��λ�õ�ֵ
 *�������������ͳ���Ϊһ���������飬
 *���дӵ͵�ַ��λ���ߵ�ַ��λ��ѯ����͵�ַ�����λ����Ϊ�±�[0]
 *��������
 * __format         -->Ŀ���ֽڵĵ�ַ
 * __size			-->Ŀ���������͵ĳ���
 * __serial__number -->Ŀ��λ���±����
 * �����ɹ����ض�ȡ��ֵ�����򷵻�-1��
 */
signed char Bit_Read(void* __format, unsigned short __size, unsigned short __serial__number) {
    unsigned short byte_num = __serial__number / BYTE_SIZE;//�ֽ���
    unsigned char byte_bit = __serial__number % BYTE_SIZE;//�ֽ��е�λ��
    if (__serial__number >= __size * BYTE_SIZE)//Խ�����
        return -1;
    unsigned char* BIT_ptr = (unsigned char*)__format;
    BIT_ptr += byte_num;//�ƶ�ָ��
    unsigned char bit = ((*BIT_ptr) & (BIT_MASK >> byte_bit)) ? 1 : 0;
    return bit;
}
/*��������������Ϊ���ģ��޸�ָ��λ�õ�ֵ
 *�������������ͳ���Ϊһ���������飬
 *���дӵ͵�ַ��λ���ߵ�ַ��λ��ѯ����͵�ַ�����λ����Ϊ�±�[0]
 *��������
 * __format         -->Ŀ���ֽڵĵ�ַ
 * __size			-->Ŀ���������͵ĳ���
 * __serial__number -->Ŀ��λ���±����
 * __binary_number  -->�޸ĺ��ֵ����ֵ��Ϊ0ʱ�����ᱻĬ���޸�Ϊ1����ֵΪ0ʱ�����޸�Ϊ0
 * �����ɹ�����0�����򷵻�-1��
 */
int Bit_Modify(void* __format, unsigned short __size, unsigned short __serial__number, unsigned char __binary_number) {
    unsigned short byte_num = __serial__number / BYTE_SIZE;//�ֽ���
    unsigned char byte_bit = __serial__number % BYTE_SIZE;//�ֽ��е�λ��

    if (__serial__number >= __size * BYTE_SIZE)//Խ�����
        return -1;

    unsigned char* BIT_ptr = (unsigned char*)__format;
    BIT_ptr += byte_num;//�ƶ�ָ��

    if (__binary_number) {
        (*BIT_ptr) |= (BIT_MASK >> byte_bit);
        return 0;
    }
    else {
        (*BIT_ptr) &= ~(BIT_MASK >> byte_bit);
        return 0;
    }
}
//-------------------------------------------------------------����-----------------------------------------------
