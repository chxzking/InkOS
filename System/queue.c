#include "queue.h"

//初始化消息队列
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

//【消息发送】
int Message_Send(Queue_t* Queue ,void* Data, Message_Type type) {
	//检查队列是否启用
	if(Queue->enable_flag == QUEUE_DISABLE){
		return -1;//队列禁用
	}
	
	//检查是否给队列头添加了自己的签名
	if (Queue->Sender == NULL) {
		//如果没有添加则立刻添加自己的签名
		Queue->Sender = Task_Current_Handle();
	}

	//判断是否选择阻塞模式，如果是则判断SR权限
	if (type == BLOCKING && Queue->SR_Flag == ONLY_ALLOWED_RECEPTION) {
		//如果是阻塞模式，并且没有发送权限,则立刻挂起
		Task_Suspended();
	}

	//如果为非阻塞模式，或者阻塞模式但是SR具有发送权限则开始操作消息队列

	//判断整个消息队列是否为空
	if (Queue->Read_Head) {
		//如果不为空，则检测上一个值和当前要发送的值是否相同
		if (!memcmp(Data, Queue->Queue_Tail->Data, Queue->Size)) {
			//如果相同,将上一个消息的count自增一个（也就是合并在一个消息中，让其可以多读取）,
			//注意这里的消息总数不要自增，因为可以留出一个空间给新的数据，提高利用效率
			if(Queue->Queue_Tail->Count+1 == 0){
				//如果这个count+1为0意味着发生了溢出，立刻返回错误值
				return -1;
			}
			Queue->Queue_Tail->Count++;
			return 0;
		}

		//如果不同，检查申请的队列空间是否被占满
		if (Queue->Queue_Max_Count == Queue->Current_Count) {
			//如果当前个数等于最大个数，那么代表消息已满
			return -1;
		}
		//如果没满，则创建新节点
	}
	//新建一个节点（与空队列情况合并）
	#if HEAP_SCHEME == HEAP_SOLUTION_1 
	Queue_Node* node = (Queue_Node*)Ink_malloc(sizeof(Queue_Node));
	#endif
	//检查队列是否申请成功
	if (!node) {
		//申请失败则立刻返回错误值
		return -1;
	}

	//为消息队列节点要传输的数据申请一个空间
	#if HEAP_SCHEME == HEAP_SOLUTION_1 
	node->Data = Ink_malloc(Queue->Size);
	#endif
	//检查数据空间申请是否成功
	if (node->Data == NULL) {
		//如果空间分配失败，立刻释放之前申请的节点
		#if HEAP_SCHEME == HEAP_SOLUTION_1 
		Ink_free(node, sizeof(Queue_Node));
		#endif
		return -1;
	}
	//如果申请成功，则初始化节点的各项数据
	node->next = NULL;
	node->Count = 1;	//记录这个信息的可读取次数为1.

	//将此刻的数据复制到队列中的数据保存空间。
	memcpy(node->Data, Data, Queue->Size);
	//将当前消息队列的总数自增
	Queue->Current_Count++;

	//节点入队，检查队列是否为空
	if (!Queue->Read_Head) {
		//情况1:队列为空，更新首节点
		Queue->Read_Head = node;
	}
	//不为空则值需要更新尾部节点为新加入的节点
	Queue->Queue_Tail = node;

	#if MESSAGE_MODE == Synchronous_Communication_Mode	
	if (type == BLOCKING && Queue->SR_Flag == ONLY_ALLOWED_TO_SEND) {
	#endif
		
		//将权限设置为只允许接收不允许发送
		Queue->SR_Flag = ONLY_ALLOWED_RECEPTION;
		//唤醒接收者任务
		Task_Wake(Queue->Receiver);
		
	#if MESSAGE_MODE == Synchronous_Communication_Mode	
	}
	#endif
	
	return 0;
}




/*【消息接收】
* 1、不阻塞，
* 3、永久阻塞，一定要收到消息才能激活任务继续执行。
*/

int Message_Receive(Queue_t* Queue, void* Data,Message_Type type) {
	//检查队列是否启用
	if(Queue->enable_flag == QUEUE_DISABLE){
		return -1;//队列禁用
	}
	
	//检查队列头中是否有自己的签名
	if (Queue->Receiver == NULL) {
		//如果没有则立刻加上自己的签名
		Queue->Receiver = Task_Current_Handle();
	}

	//判断是否选择阻塞模式，如果是则判断SR权限
	if (type == BLOCKING && Queue->SR_Flag == ONLY_ALLOWED_TO_SEND) {
		//如果是阻塞模式，并且没有接收权限,则立刻挂起
		Task_Suspended();
	}

	//检查消息队列是否为空
	if (!Queue->Read_Head) {
		//如果消息队列为空，则立刻返回结果
		return -1;
	}

	//检查消息队列的可读取次数是否大于1次
	if (Queue->Read_Head->Count > 1) {
		//如果可以读取多次,将消息队列中的数据传输过来
		memcpy(Data, Queue->Read_Head->Data, Queue->Size);
		//减少可读取数量
		Queue->Read_Head->Count--;
		//检查是否设置了阻塞模式，以及是为仅接收权限
		if (type == BLOCKING && Queue->SR_Flag == ONLY_ALLOWED_RECEPTION) {
			//将权限设置为只允许发送不允许接收
			Queue->SR_Flag = ONLY_ALLOWED_TO_SEND;
			//唤醒发送者任务
			Task_Wake(Queue->Sender);
		}
		return 0;
	}

	//如果消息队列的可读取次数没有超过1次（不可能等于0）
	//定义一个临时变量用于记录当前的队列
	Queue_Node* temp = Queue->Read_Head;
	//将当前的队列指针移动到下一个节点
	Queue->Read_Head = Queue->Read_Head->next;
	//将队列中的消息保存到当前的值中
	memcpy(Data, temp->Data, Queue->Size);
	//释放队列空间
	#if HEAP_SCHEME == HEAP_SOLUTION_1 
	Ink_free(temp->Data,Queue->Size);
	Ink_free(temp, sizeof(Queue_Node));
	#endif
	//将消息队列的总数减少，同时判断是否当前的队列总数为0，
	if (--Queue->Current_Count) {
		//如果为0，则立刻将指针置空
		Queue->Queue_Tail = NULL;
		Queue->Read_Head = NULL;
	}

	/*设置为无论那种情况经过都会唤醒发送者任务
	*优点： 实现了发送和接收双方可以保持阻塞与非阻塞的不同模式
	*缺点：额外进行了唤醒权限修改不必要的操作，损失部分性能
	*/
	#if MESSAGE_MODE == Synchronous_Communication_Mode	
	if (type == BLOCKING && Queue->SR_Flag == ONLY_ALLOWED_RECEPTION) {
	#endif
		//将权限设置为只允许发送不允许接收
		Queue->SR_Flag = ONLY_ALLOWED_TO_SEND;
		//唤醒发送者任务
		Task_Wake(Queue->Sender);
	#if MESSAGE_MODE == Synchronous_Communication_Mode	
	}
	#endif
	return 0;
}


//队列释放
int Queue_Delete(Queue_t* Queue){
	//检查队列指针是否为空
	if(!Queue){
		//为空则直接返回
		return 0;
	}
	//检查队列是否为空
	if(!Queue->Read_Head){
		//如果为空，直接销毁队列头
		#if HEAP_SCHEME == HEAP_SOLUTION_1 
		Ink_free(Queue,sizeof(Queue_t));
		#endif
		return 0;
	}
	//不为空则逐个销毁节点
	Queue_Node* temp = Queue->Read_Head;
	do{
		Queue->Read_Head = Queue->Read_Head->next;//更新节点
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


//消息队列启动
void Message_Activate(Queue_t* Queue){
	Queue->enable_flag = QUEUE_ENABLE;
	return;
}
//消息队列休眠
void Message_Sleep(Queue_t* Queue){
	Queue->enable_flag = QUEUE_DISABLE;
	return;
}
