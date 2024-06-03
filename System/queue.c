#include "queue.h"

//��ʼ����Ϣ����
Queue_t* MessageQueue_Init(unsigned int Queue_Count,unsigned int Size) {
	#if HEAP_SCHEME == HEAP_SOLUTION_1  
	Queue_t* temp = (Queue_t*)Ink_malloc(sizeof(Queue_t)) ;
	#endif
	
	if(!temp){
		return temp;
	}
	temp->Queue_Max_Count = Queue_Count;
	temp->Size = Size;
	temp->Current_Count = 0;
	temp->SR_Flag = ONLY_ALLOWED_TO_SEND;
	temp->Queue_Tail = NULL;
	temp->Read_Head = NULL;
	temp->Sender = NULL;
	temp->Receiver = NULL;
	temp->enable_flag = QUEUE_ENABLE;
	return temp;
}

//����Ϣ���͡�
int Message_Send(Queue_t* Queue ,void* Data, Message_Type type) {
	//�������Ƿ�����
	if(Queue->enable_flag == QUEUE_DISABLE){
		return -1;//���н���
	}
	
	//����Ƿ������ͷ������Լ���ǩ��
	if (Queue->Sender == NULL) {
		//���û���������������Լ���ǩ��
		Queue->Sender = Task_Current_Handle();
	}

	//�ж��Ƿ�ѡ������ģʽ����������ж�SRȨ��
	if (type == BLOCKING && Queue->SR_Flag == ONLY_ALLOWED_RECEPTION) {
		//���������ģʽ������û�з���Ȩ��,�����̹���
		Task_Suspended();
	}

	//���Ϊ������ģʽ����������ģʽ����SR���з���Ȩ����ʼ������Ϣ����

	//�ж�������Ϣ�����Ƿ�Ϊ��
	if (Queue->Read_Head) {
		//�����Ϊ�գ�������һ��ֵ�͵�ǰҪ���͵�ֵ�Ƿ���ͬ
		if (!memcmp(Data, Queue->Queue_Tail->Data, Queue->Size)) {
			//�����ͬ,����һ����Ϣ��count����һ����Ҳ���Ǻϲ���һ����Ϣ�У�������Զ��ȡ��,
			//ע���������Ϣ������Ҫ��������Ϊ��������һ���ռ���µ����ݣ��������Ч��
			if(Queue->Queue_Tail->Count+1 == 0){
				//������count+1Ϊ0��ζ�ŷ�������������̷��ش���ֵ
				return -1;
			}
			Queue->Queue_Tail->Count++;
			return 0;
		}

		//�����ͬ���������Ķ��пռ��Ƿ�ռ��
		if (Queue->Queue_Max_Count == Queue->Current_Count) {
			//�����ǰ������������������ô������Ϣ����
			return -1;
		}
		//���û�����򴴽��½ڵ�
	}
	//�½�һ���ڵ㣨��ն�������ϲ���
	#if HEAP_SCHEME == HEAP_SOLUTION_1 
	Queue_Node* node = (Queue_Node*)Ink_malloc(sizeof(Queue_Node));
	#endif
	//�������Ƿ�����ɹ�
	if (!node) {
		//����ʧ�������̷��ش���ֵ
		return -1;
	}

	//Ϊ��Ϣ���нڵ�Ҫ�������������һ���ռ�
	#if HEAP_SCHEME == HEAP_SOLUTION_1 
	node->Data = Ink_malloc(Queue->Size);
	#endif
	//������ݿռ������Ƿ�ɹ�
	if (node->Data == NULL) {
		//����ռ����ʧ�ܣ������ͷ�֮ǰ����Ľڵ�
		#if HEAP_SCHEME == HEAP_SOLUTION_1 
		Ink_free(node, sizeof(Queue_Node));
		#endif
		return -1;
	}
	//�������ɹ������ʼ���ڵ�ĸ�������
	node->next = NULL;
	node->Count = 1;	//��¼�����Ϣ�Ŀɶ�ȡ����Ϊ1.

	//���˿̵����ݸ��Ƶ������е����ݱ���ռ䡣
	memcpy(node->Data, Data, Queue->Size);
	//����ǰ��Ϣ���е���������
	Queue->Current_Count++;

	//�ڵ���ӣ��������Ƿ�Ϊ��
	if (!Queue->Read_Head) {
		//���1:����Ϊ�գ������׽ڵ�
		Queue->Read_Head = node;
	}
	//��Ϊ����ֵ��Ҫ����β���ڵ�Ϊ�¼���Ľڵ�
	Queue->Queue_Tail = node;

	#if MESSAGE_MODE == Synchronous_Communication_Mode	
	if (type == BLOCKING && Queue->SR_Flag == ONLY_ALLOWED_TO_SEND) {
	#endif
		
		//��Ȩ������Ϊֻ������ղ�������
		Queue->SR_Flag = ONLY_ALLOWED_RECEPTION;
		//���ѽ���������
		Task_Wake(Queue->Receiver);
		
	#if MESSAGE_MODE == Synchronous_Communication_Mode	
	}
	#endif
	
	return 0;
}




/*����Ϣ���ա�
* 1����������
* 3������������һ��Ҫ�յ���Ϣ���ܼ����������ִ�С�
*/

int Message_Receive(Queue_t* Queue, void* Data,Message_Type type) {
	//�������Ƿ�����
	if(Queue->enable_flag == QUEUE_DISABLE){
		return -1;//���н���
	}
	
	//������ͷ���Ƿ����Լ���ǩ��
	if (Queue->Receiver == NULL) {
		//���û�������̼����Լ���ǩ��
		Queue->Receiver = Task_Current_Handle();
	}

	//�ж��Ƿ�ѡ������ģʽ����������ж�SRȨ��
	if (type == BLOCKING && Queue->SR_Flag == ONLY_ALLOWED_TO_SEND) {
		//���������ģʽ������û�н���Ȩ��,�����̹���
		Task_Suspended();
	}

	//�����Ϣ�����Ƿ�Ϊ��
	if (!Queue->Read_Head) {
		//�����Ϣ����Ϊ�գ������̷��ؽ��
		return -1;
	}

	//�����Ϣ���еĿɶ�ȡ�����Ƿ����1��
	if (Queue->Read_Head->Count > 1) {
		//������Զ�ȡ���,����Ϣ�����е����ݴ������
		memcpy(Data, Queue->Read_Head->Data, Queue->Size);
		//���ٿɶ�ȡ����
		Queue->Read_Head->Count--;
		//����Ƿ�����������ģʽ���Լ���Ϊ������Ȩ��
		if (type == BLOCKING && Queue->SR_Flag == ONLY_ALLOWED_RECEPTION) {
			//��Ȩ������Ϊֻ�����Ͳ��������
			Queue->SR_Flag = ONLY_ALLOWED_TO_SEND;
			//���ѷ���������
			Task_Wake(Queue->Sender);
		}
		return 0;
	}

	//�����Ϣ���еĿɶ�ȡ����û�г���1�Σ������ܵ���0��
	//����һ����ʱ�������ڼ�¼��ǰ�Ķ���
	Queue_Node* temp = Queue->Read_Head;
	//����ǰ�Ķ���ָ���ƶ�����һ���ڵ�
	Queue->Read_Head = Queue->Read_Head->next;
	//�������е���Ϣ���浽��ǰ��ֵ��
	memcpy(Data, temp->Data, Queue->Size);
	//�ͷŶ��пռ�
	#if HEAP_SCHEME == HEAP_SOLUTION_1 
	Ink_free(temp->Data,Queue->Size);
	Ink_free(temp, sizeof(Queue_Node));
	#endif
	//����Ϣ���е��������٣�ͬʱ�ж��Ƿ�ǰ�Ķ�������Ϊ0��
	if (--Queue->Current_Count) {
		//���Ϊ0�������̽�ָ���ÿ�
		Queue->Queue_Tail = NULL;
		Queue->Read_Head = NULL;
	}

	/*����Ϊ������������������ỽ�ѷ���������
	*�ŵ㣺 ʵ���˷��ͺͽ���˫�����Ա���������������Ĳ�ͬģʽ
	*ȱ�㣺��������˻���Ȩ���޸Ĳ���Ҫ�Ĳ�������ʧ��������
	*/
	#if MESSAGE_MODE == Synchronous_Communication_Mode	
	if (type == BLOCKING && Queue->SR_Flag == ONLY_ALLOWED_RECEPTION) {
	#endif
		//��Ȩ������Ϊֻ�����Ͳ��������
		Queue->SR_Flag = ONLY_ALLOWED_TO_SEND;
		//���ѷ���������
		Task_Wake(Queue->Sender);
	#if MESSAGE_MODE == Synchronous_Communication_Mode	
	}
	#endif
	return 0;
}


//�����ͷ�
int Queue_Delete(Queue_t* Queue){
	//������ָ���Ƿ�Ϊ��
	if(!Queue){
		//Ϊ����ֱ�ӷ���
		return 0;
	}
	//�������Ƿ�Ϊ��
	if(!Queue->Read_Head){
		//���Ϊ�գ�ֱ�����ٶ���ͷ
		#if HEAP_SCHEME == HEAP_SOLUTION_1 
		Ink_free(Queue,sizeof(Queue_t));
		#endif
		return 0;
	}
	//��Ϊ����������ٽڵ�
	Queue_Node* temp = Queue->Read_Head;
	do{
		Queue->Read_Head = Queue->Read_Head->next;//���½ڵ�
		#if HEAP_SCHEME == HEAP_SOLUTION_1 
		Ink_free(temp->Data,Queue->Size);
		Ink_free(temp,sizeof(Queue_Node));
		#endif
		temp = Queue->Read_Head;
	}while(Queue->Read_Head != NULL);
	
	#if HEAP_SCHEME == HEAP_SOLUTION_1 
	Ink_free(Queue,sizeof(Queue_t));
	#endif
	return 0;
}


//��Ϣ��������
void Message_Activate(Queue_t* Queue){
	Queue->enable_flag = QUEUE_ENABLE;
	return;
}
//��Ϣ��������
void Message_Sleep(Queue_t* Queue){
	Queue->enable_flag = QUEUE_DISABLE;
	return;
}
