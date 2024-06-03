#include "task.h"
//定义隐藏在本文件中的数据
//变量
SCHEDULING_PRIORITY_CHAIN* SPC_Head = NULL;                                     //SPC头节点
SCHEDULING_PRIORITY_CHAIN* SPC_Detector = NULL;                                 //SPC探头
INSPECTOR SPC_TOR_Inspector[2];                                                 //双巡查员(第一位为完全巡查，第二位活跃巡查)
HashTable Hash_TOR;                                                             //定义一个针对TOR名称和位置关联的hash表
unsigned char TaskQueueUpdated = NEW_TASK_NOT_AVAILABLE;                        //是否有新任务加入的标志位（新加载任务或唤醒任务）
Task_Wait_Tracker* TWT_Header = NULL;											//等待队列头指针
Stack_Overflow_t STACK_OVERFLOW_FLAG = STACK_SAFETY;											//栈溢出信号

//空闲任务
unsigned int Task_Idle_Stack[TASK_IDLE_STACK_SIZE] = { 0 };
TASK_ORBIT_RING TOR_Idle;
SCHEDULING_PRIORITY_CHAIN SPC_Idle;

//函数
int SPC_Add(unsigned char Priority);                                            //添加
int SPC_Delete(unsigned char Priority);                                         //删除
int TOR_Create(TASK_FUNCTION taskFunction, \
    void* Args, \
    unsigned char Priority, \
    unsigned char* icon, \
    char* Task_name, \
    StackDepth size, \
    TASK_ORBIT_RING** node);                                                    //添加一个TOR节点(!!!!!!!!!!!注意，只是添加了一个TOR，并没有在哈希表中添加节点，目前规划在未来的Task_Create中进行两者结合)

int TOR_Move(TASK_ORBIT_RING* TOR);                                             //TOR移动（优先级出现改变后，从当前SPC移动到指定优先级的SPC）

/*************************
* 巡查员定义
**************************/
///////////////////////////////////////////////////////////开始//////////////////////////////////////////////////////////    
//初始化巡查
void Inspector_Init(void) {
    //完全巡查（对象是所有的任务）
    //活跃巡查（对象是被启用【加载进入内存的任务】）
    /*
    * 巡查员初始化为两个NULL的想法如下：
    *【1】 在正常情况下，巡查员的SPC区在经历了第一次执行后就不可能再出现等于NULL的情况，
    因为在查询的函数中，出现了NULL就会被立刻更新为其他符合要求的SPC地址，
    如果返回结果出现了SPC=NULL那么就代表这SPC链出现的了问题，导致返回NULL，即{用于判断SPC链是否正常}
    * 【2】SPC异常的两种情况，
    1、SPC没有节点，那么巡查首次移动就是NULL，此时立刻返回巡查员。
    2、SPC经过遍历，不存在有效SPC节点
    */
    for (unsigned char i = 0; i < 2; i++) {
        SPC_TOR_Inspector[i].SPC = NULL;
        SPC_TOR_Inspector[i].TOR = NULL;
    }
}
//正向完全巡查(巡查员立刻更新为一个节点,并返回该值)
INSPECTOR Complete_Inspector_Forword(void) {
    //首次执行
    if (SPC_TOR_Inspector[0].SPC == NULL) {
        SPC_TOR_Inspector[0].SPC = SPC_Head->next;//将巡查员移动到首个SPC节点。
        //那么寻找第一个有效（即悬挂了TOR的节点，注意在SPC链中有效节点远远多余无效节点）的SPC节点
        while (SPC_TOR_Inspector[0].SPC == NULL || SPC_TOR_Inspector[0].SPC->Ring_Task_Amount == 0) {
            /*这个条件中的NULL是判断如下两种情况，
            * （1）SPC链为空，那么就会出现SPC_Head->next=NULL的情况
            * （2）SPC经过遍历，发现所有的SPC都是无效SPC节点，导致巡查员到尾部出现异常。
            */
            if (SPC_TOR_Inspector[0].SPC == NULL) {
                //立刻返回包含错误的巡查结果
                return SPC_TOR_Inspector[0];
            }
            SPC_TOR_Inspector[0].SPC = SPC_TOR_Inspector[0].SPC->next;
        }
        //默认读取SPC中TOR的环首
        SPC_TOR_Inspector[0].TOR = SPC_TOR_Inspector[0].SPC->TOR_Leader;
        return SPC_TOR_Inspector[0];
    }


    //非首次执行
    /*
    * 非首次执行后意味着，SPC的在正常情况下不会再出现空链表和无有效SPC的情况，所以不需要再考虑巡查员出现错误的问题
    * 巡查员不会停留在SPC上，它的主要目标是TOR，所以之后的核心是TOR的问题以及跨SPC和同SPC遍历
    */

    //情况1（同SPC遍历）,在环节点大于1的TOR中，不处于环尾，巡查移动到同环的下一个位置
    if (SPC_TOR_Inspector[0].SPC->Ring_Task_Amount > 1 &&//判断环节点数大于1
        SPC_TOR_Inspector[0].TOR->next != SPC_TOR_Inspector[0].SPC->TOR_Leader) {//判断环节点不是环尾
        SPC_TOR_Inspector[0].TOR = SPC_TOR_Inspector[0].TOR->next;//巡查移动到下一个位置
    }

    //情况2（跨SPC遍历，此时已经处于TOR环的临界点），如果环的节点数大于1且处于环尾  或者环节点总数为1，都更新spc并指向新SPC中的环首
    else if ((SPC_TOR_Inspector[0].SPC->Ring_Task_Amount > 1 &&//临界点1：TOR环的节点数大于1，但是当前已经遍历到了SPC的环尾，即将更换新的SPC
        SPC_TOR_Inspector[0].TOR->next == SPC_TOR_Inspector[0].SPC->TOR_Leader) ||
        //临界点2：TOR环的数量只有1个，那么处于TOR环首的同时也处于环尾，所以即将更新SPC
        SPC_TOR_Inspector[0].SPC->Ring_Task_Amount == 1) {
        /*寻找到合适的SPC节点，先移动到一个SPC，判断其是否有效，有效返回，无效则持续循环查找，直到出现符合要求的新SPC
        *（由于之前已经出现了有效的SPC，所以不管如何遍历一定会再次出现有效的SPC而不可能出现无效节点）
        */
        do {
            //判断下一个节点是否是SPC尾部NULL区，如果是则立刻回到SPC第一个节点，并结束本轮循环
            if (SPC_TOR_Inspector[0].SPC->next == NULL) {
                SPC_TOR_Inspector[0].SPC = SPC_Head->next;
                continue;
            }
            SPC_TOR_Inspector[0].SPC = SPC_TOR_Inspector[0].SPC->next;
        } while (SPC_TOR_Inspector[0].SPC == NULL || SPC_TOR_Inspector[0].SPC->Ring_Task_Amount == 0);
        //刷新到新SPC的环首
        SPC_TOR_Inspector[0].TOR = SPC_TOR_Inspector[0].SPC->TOR_Leader;
    }
    return SPC_TOR_Inspector[0];
}
//逆向完全巡查(巡查员立刻回退到上一个节点,返回该值)
INSPECTOR Complete_Inspector_Reverse(void) {
    //首次执行
    if (SPC_TOR_Inspector[0].SPC == NULL) {
        SPC_TOR_Inspector[0].SPC = SPC_Head->previous;//将巡查员移动到最后一个SPC节点。
        //寻找第一个逆向有效（即悬挂了TOR的节点，注意在SPC链中有效节点远远多余无效节点）的SPC节点
        while (SPC_TOR_Inspector[0].SPC == NULL || SPC_TOR_Inspector[0].SPC->Ring_Task_Amount == 0 || SPC_TOR_Inspector[0].SPC == SPC_Head) {
            //当SPC链为空或者SPC链的不存在有效节点的时候立刻返回
            if (SPC_TOR_Inspector[0].SPC == NULL || SPC_TOR_Inspector[0].SPC == SPC_Head) {
                SPC_TOR_Inspector[0].SPC = NULL;//统一设置为NULL，用于表示巡查员出现问题
                return SPC_TOR_Inspector[0];//SPC链存在问题
            }
            SPC_TOR_Inspector[0].SPC = SPC_TOR_Inspector[0].SPC->previous;
        }
        //锁定有效SPC节点后，将TOR更新为环尾，在这个情况下如果TOR环只存在一个节点，依旧能正确匹配到合适的TOR节点
        SPC_TOR_Inspector[0].TOR = SPC_TOR_Inspector[0].SPC->TOR_Leader->previous;
        return SPC_TOR_Inspector[0];
    }

    //非首次执行
    //情况1（同SPC遍历）,在环节点大于1的TOR中，不处于环首，巡查移动到同环的下一个位置
    if (SPC_TOR_Inspector[0].SPC->Ring_Task_Amount > 1 &&//判断环节点数大于1
        SPC_TOR_Inspector[0].TOR->previous != SPC_TOR_Inspector[0].SPC->TOR_Leader->previous) {//判断环节点不是环首
        SPC_TOR_Inspector[0].TOR = SPC_TOR_Inspector[0].TOR->previous;//巡查移动到下一个位置
    }

    //情况2（跨SPC遍历，此时已经处于TOR环的临界点），如果环的节点数大于1且处于环首  或者环节点总数为1，都更新spc并指向新SPC中的环尾
    else if ((SPC_TOR_Inspector[0].SPC->Ring_Task_Amount > 1 &&//临界点1：在TOR节点大于1的情况下并且处于环首
        SPC_TOR_Inspector[0].TOR->previous == SPC_TOR_Inspector[0].SPC->TOR_Leader->previous) ||
        //临界点2，TOR环数量为1，此时的TOR即是环首又是环尾
        SPC_TOR_Inspector[0].SPC->Ring_Task_Amount == 1) {
        //寻找到合适的SPC节点
        do {
            //当SPC节点遍历到SPC头结点的时候，也就是链表遍历换成，则跳回到链表的尾部
            if (SPC_TOR_Inspector[0].SPC->previous == SPC_Head) {
                SPC_TOR_Inspector[0].SPC = SPC_Head->previous;
                continue;
            }
            SPC_TOR_Inspector[0].SPC = SPC_TOR_Inspector[0].SPC->previous;
        } while (SPC_TOR_Inspector[0].SPC == SPC_Head || SPC_TOR_Inspector[0].SPC->Ring_Task_Amount == 0);
        SPC_TOR_Inspector[0].TOR = SPC_TOR_Inspector[0].SPC->TOR_Leader->previous;//刷新到新SPC的环尾
    }
    return SPC_TOR_Inspector[0];
}
//静式完全巡查
INSPECTOR Complete_Inspector_Static(void) {
    //巡查员不进行任务移动，保持原来的状态
    return SPC_TOR_Inspector[0];
}

//正向活跃巡查(巡查员立刻更新为一个节点,并返回该值)
INSPECTOR Active_Inspector_Forword(void){
	//首次执行
	/*在初始状态下巡查员被设置为NULL
	*/
	if(SPC_TOR_Inspector[1].SPC == NULL){
		//如果SPC链没有任务SPC节点则直接返回处于初始化状态的巡查员，
		//改值可以作为异常情况的判断
		if(SPC_Head->next == NULL){
			return SPC_TOR_Inspector[1];
		}
		
		//SPC链有SPC节点，那么从第一个节点开始寻找合适的SPC节点
		SPC_TOR_Inspector[1].SPC = SPC_Head->next;
		while(SPC_TOR_Inspector[1].SPC != NULL && SPC_TOR_Inspector[1].SPC->Ring_Active_Amount == 0){
			SPC_TOR_Inspector[1].SPC  = SPC_TOR_Inspector[1].SPC ->next;
		}
		
		//分析结果
		//结果1：遍历整条SPC，没有发现任何具有活跃TOR的SPC，返回可以被识别为异常的结果
		if(SPC_TOR_Inspector[1].SPC == NULL){
			return SPC_TOR_Inspector[1];
		}
		
		//结果2：找到了具有至少1个活跃TOR的SPC
		SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].SPC->TOR_Leader;
		/*此处遍历TOR存在极端情况
		*（1）在执行整个TOR环遍历的过程中，没有任务被禁用或者被挂起
		*（2）当遍历到符合要求的TOR节点时，即将返回这个结果，但是在此瞬间任务切换，恰好禁用或者挂起来该任务
		*/
		do{
			//如果处于活跃态则返回结果
			SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].TOR->next;
		}while(SPC_TOR_Inspector[1].TOR != SPC_TOR_Inspector[1].SPC->TOR_Leader &&\
				SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_DISABLE_FLAG && \
		TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics) == TASK_OBSTRUCT);
		
		//分析TOR查找结果
		/*情况1：找到一个符合要求的tor
		*情况2：在没有找到tor之前这个tor退出了活跃态，再次遍历到tor环首后被退出
		*情况3：找到了tor环，但是找到之后还没有返回就退出了活跃态
		*/
		if(SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_ENABLE_FLAG && TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics) != TASK_OBSTRUCT){
			//理想情况1
			return SPC_TOR_Inspector[1];
		}else{
			//情况2、3合并为一种情况，重置巡查员的值为可识别出异常的NULL
			SPC_TOR_Inspector[1].SPC = NULL;
			SPC_TOR_Inspector[1].TOR = NULL;
			return SPC_TOR_Inspector[1];
		}
		
	
	}
	//非首次执行
	
	/*【大类讨论】
	*情况1：在同一个TOR环中具有2个及以上的节点
	*（1）当前TOR节点在环首
	*（2）当前TOR节点在环中，其他活跃节点至少有一个在当前TOR节点和尾节点之间。
	*（3）当前TOR节点在环中，但是当前TOR节点和尾节点中不存在活跃TOR节点
	*（4）当前TOR节点就是环尾。
	*情况2：
	*（1）当前TOR环有且仅有当前选中的一个节点，如果在判断过后当前有一个节点被唤醒，当前的巡查员不需要负责。
	*/
	
	//【大类情况1】
	if(SPC_TOR_Inspector[1].SPC->Ring_Active_Amount > 1 ){	
		//用一个临时变量存储当前巡查员的TOR值
		TASK_ORBIT_RING* temp_TOR = SPC_TOR_Inspector[1].TOR;
		//将巡查员重新拉回TOR环环首
		SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].SPC->TOR_Leader;
		//定义一个巡查员经过上一个TOR节点的标志
		unsigned char pass_flag = 0;
		
		do{	
			/*经过标志激活，意味着巡查员已经到达了之前的环和环尾中间的位置
			*如果选中的TOR处于启用状态，处于活跃态则可以返回当前的结果
			*如果在判断通过到返回的临界点，该TOR节点被挂起或者禁用，巡查员不需要对此负责
			*/
			if(SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_ENABLE_FLAG && pass_flag && TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics)!= TASK_OBSTRUCT){
				return SPC_TOR_Inspector[1];
			}
			/*此处遍历到当前的TOR节点，更新pass_flag标志，代码位置不能更改，
			*放在下面的作用是：避免重复返回当前TOR节点，
			*如果遍历到了当前TOR时，由于pass_flag还处于0状态，所以会被跳过
			*/
			if(SPC_TOR_Inspector[1].TOR == temp_TOR){
				pass_flag = 1;
			}
			SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].TOR->next;
		}while(SPC_TOR_Inspector[1].TOR != SPC_TOR_Inspector[1].SPC->TOR_Leader);
		
		/*执行到此处意味着当前的TOR环出现问题
		*（1）当前的TOR和环尾之间不存在活跃的TOR，
		*（2）当前的TOR处于环尾
		*【解决方案】
		*寻找新的SPC节点，所以需要跳转到【大类情况2】
		*/
		goto situation1_flag;
		
		
	}else{
	//【大类情况2】
		situation1_flag:;//此标志从if跳转过来
		situation2_flag:;//此标志在下部分往上部分跳转
		
		//从当前SPC往尾部正向遍历
		//这里不用担心死循环的问题，因为调用这个函数就以为着SPC链中必定有至少一个TOR是活跃的
		do{
			if(SPC_TOR_Inspector[1].SPC->next == NULL){
				//如果遍历到了SPC链末尾，则跳回第一个节点
				SPC_TOR_Inspector[1].SPC = SPC_Head->next;
				continue;
			}
			SPC_TOR_Inspector[1].SPC = SPC_TOR_Inspector[1].SPC->next;//移动SPC节点
			//判断条件：遇到具有活跃节点的SPC就停止
		}while(SPC_TOR_Inspector[1].SPC->Ring_Active_Amount == 0);
		
		
		//当前结果是一个包含了活跃TOR的SPC，所以将巡查员的TOR设置为环首，并从此处开始遍历环
		SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].SPC->TOR_Leader;
		
		do{
			//如果当前的TOR被启用且不处于阻塞态
			if(SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_ENABLE_FLAG && TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics) != TASK_OBSTRUCT){
				/*此处判断，当发现了一个TOR节点开启并处于活跃状态即可返回，
				*如果返回之后，该节点在这个返回的临界点被挂起或者被禁用，巡查员也不需要对此负责
				*/
				return SPC_TOR_Inspector[1];
			}
			SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].TOR->next;
		}while(SPC_TOR_Inspector[1].TOR != SPC_TOR_Inspector[1].SPC->TOR_Leader);
		
		/*如果代码执行到此处，意味着在遍历期间，该SPC的活跃节点已经不再处于活跃态，
		*所以需要重新选择一个SPC节点。
		*/
		goto situation2_flag;
		
	}
		
}
//逆向活跃巡查(巡查员立刻回退到上一个节点,返回该值)
INSPECTOR Active_Inspector_Reverse(void){
	//首次执行
	/*在初始状态下巡查员被设置为NULL
	*/
	if(SPC_TOR_Inspector[1].SPC == NULL){
		//如果SPC链没有任务SPC节点则直接返回处于初始化状态的巡查员，
		//该值可以作为异常情况的判断
		if(SPC_Head->previous == NULL){
			return SPC_TOR_Inspector[1];
		}
		
		//SPC链有SPC节点，那么从最后一个节点开始寻找合适的SPC节点
		SPC_TOR_Inspector[1].SPC = SPC_Head->previous;
		while(SPC_TOR_Inspector[1].SPC != SPC_Head && SPC_TOR_Inspector[1].SPC->Ring_Active_Amount == 0){
			SPC_TOR_Inspector[1].SPC  = SPC_TOR_Inspector[1].SPC ->previous;
		}
		
		//分析结果
		//结果1：遍历整条SPC，没有发现任何具有活跃TOR的SPC，返回可以被识别为异常的结果
		if(SPC_TOR_Inspector[1].SPC == SPC_Head){
			SPC_TOR_Inspector[1].SPC = NULL;
			SPC_TOR_Inspector[1].TOR = NULL; 
			return SPC_TOR_Inspector[1];
		}
		
		//结果2：找到了具有至少1个活跃TOR的SPC
		SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].SPC->TOR_Leader->previous;
		/*此处遍历TOR存在极端情况
		*（1）在执行整个TOR环遍历的过程中，没有任务被禁用或者被挂起
		*（2）当遍历到符合要求的TOR节点时，即将返回这个结果，但是在此瞬间任务切换，恰好禁用或者挂起来该任务
		*/
		do{
			//如果处于活跃态则返回结果
			SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].TOR->previous;
		}while(SPC_TOR_Inspector[1].TOR != SPC_TOR_Inspector[1].SPC->TOR_Leader->previous &&\
				SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_DISABLE_FLAG && \
		TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics) == TASK_OBSTRUCT);
		
		//分析TOR查找结果
		/*情况1：找到一个符合要求的tor
		*情况2：在没有找到tor之前这个tor退出了活跃态，再次遍历到tor环首后被退出
		*情况3：找到了tor环，但是找到之后还没有返回就退出了活跃态
		*/
		if(SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_ENABLE_FLAG && TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics) != TASK_OBSTRUCT){
			//理想情况1
			return SPC_TOR_Inspector[1];
		}else{
			//情况2、3合并为一种情况，重置巡查员的值为可识别出异常的NULL
			SPC_TOR_Inspector[1].SPC = NULL;
			SPC_TOR_Inspector[1].TOR = NULL;
			return SPC_TOR_Inspector[1];
		}
		
	
	}
	//非首次执行
	
	/*【大类讨论】
	*情况1：在同一个TOR环中具有2个及以上的节点
	*（1）当前TOR节点在环首
	*（2）当前TOR节点在环中，其他活跃节点至少有一个在当前TOR节点和尾节点之间。
	*（3）当前TOR节点在环中，但是当前TOR节点和尾节点中不存在活跃TOR节点
	*（4）当前TOR节点就是环尾。
	*情况2：
	*（1）当前TOR环有且仅有当前选中的一个节点，如果在判断过后当前有一个节点被唤醒，当前的巡查员不需要负责。
	*/
	
	//【大类情况1】
	if(SPC_TOR_Inspector[1].SPC->Ring_Active_Amount > 1 ){	
		//用一个临时变量存储当前巡查员的TOR值
		TASK_ORBIT_RING* temp_TOR = SPC_TOR_Inspector[1].TOR;
		//将巡查员重新拉回TOR环环尾
		SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].SPC->TOR_Leader->previous;
		//定义一个巡查员经过上一个TOR节点的标志
		unsigned char pass_flag = 0;
		
		do{	
			/*经过标志激活，意味着巡查员已经到达了之前的环和环尾中间的位置
			*如果选中的TOR处于启用状态，处于活跃态则可以返回当前的结果
			*如果在判断通过到返回的临界点，该TOR节点被挂起或者禁用，巡查员不需要对此负责
			*/
			if(SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_ENABLE_FLAG && pass_flag && TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics)!= TASK_OBSTRUCT){
				return SPC_TOR_Inspector[1];
			}
			/*此处遍历到当前的TOR节点，更新pass_flag标志，代码位置不能更改，
			*放在下面的作用是：避免重复返回当前TOR节点，
			*如果遍历到了当前TOR时，由于pass_flag还处于0状态，所以会被跳过
			*/
			if(SPC_TOR_Inspector[1].TOR == temp_TOR){
				pass_flag = 1;
			}
			SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].TOR->previous;
		}while(SPC_TOR_Inspector[1].TOR != SPC_TOR_Inspector[1].SPC->TOR_Leader->previous);
		
		/*执行到此处意味着当前的TOR环出现问题
		*（1）当前的TOR和环尾之间不存在活跃的TOR，
		*（2）当前的TOR处于环尾
		*【解决方案】
		*寻找新的SPC节点，所以需要跳转到【大类情况2】
		*/
		goto situation1_flag;
		
		
	}else{
	//【大类情况2】
		situation1_flag:;//此标志从if跳转过来
		situation2_flag:;//此标志在下部分往上部分跳转
		
		//从当前SPC往头部正向逆遍历
		//这里不用担心死循环的问题，因为调用这个函数就以为着SPC链中必定有至少一个TOR是活跃的
		do{
			if(SPC_TOR_Inspector[1].SPC->next == SPC_Head){
				//如果遍历到了SPC链首，则跳回最后一个节点
				SPC_TOR_Inspector[1].SPC = SPC_Head->previous;
				continue;
			}
			SPC_TOR_Inspector[1].SPC = SPC_TOR_Inspector[1].SPC->previous;//移动SPC节点
			//判断条件：遇到具有活跃节点的SPC就停止
		}while(SPC_TOR_Inspector[1].SPC->Ring_Active_Amount == 0);
		
		
		//当前结果是一个包含了活跃TOR的SPC，所以将巡查员的TOR设置为环首，并从此处开始遍历环
		SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].SPC->TOR_Leader->previous;
		
		do{
			//如果当前的TOR被启用且不处于阻塞态
			if(SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_ENABLE_FLAG && TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics) != TASK_OBSTRUCT){
				/*此处判断，当发现了一个TOR节点开启并处于活跃状态即可返回，
				*如果返回之后，该节点在这个返回的临界点被挂起或者被禁用，巡查员也不需要对此负责
				*/
				return SPC_TOR_Inspector[1];
			}
			SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].TOR->previous;
		}while(SPC_TOR_Inspector[1].TOR != SPC_TOR_Inspector[1].SPC->TOR_Leader->previous);
		
		/*如果代码执行到此处，意味着在遍历期间，该SPC的活跃节点已经不再处于活跃态，
		*所以需要重新选择一个SPC节点。
		*/
		goto situation2_flag;
		
	}
}
//静式活跃巡查
INSPECTOR Active_Inspector_Static(void){
	return SPC_TOR_Inspector[1];
}
/////////////////////////////////////////////////////////结束//////////////////////////////////////////////////////////    


/*************************
* 优先级基础
**************************/
///////////////////////////////////////////////////////////开始//////////////////////////////////////////////////////////    

//指定轨道环节点优先级设置
int TOR_Priority_Set(TASK_ORBIT_RING* TOR, unsigned char Priority) {
    if (!TOR)//如果TOR为空则返回错误
        return -1;
    if (Priority > 15)//优先级设置超过最大值，则设置为最大值
        Priority = 15;
    //清空原优先级
    TOR->tactics &= 0xf0;
    //写入新优先级
    TOR->tactics += Priority;
    return 0;
}
//查看指定轨道环节点的优先级
int TOR_Priority_Get(TASK_ORBIT_RING* TOR) {
    if (!TOR)//如果TOR为NULL，返回错误码
        return -1;
    return TOR->tactics & PRIORITY_MASK;
}
///////////////////////////////////////////////////////////结束//////////////////////////////////////////////////////////    
/*************************
* 任务状态设置基础
**************************/
///////////////////////////////////////////////////////////开始//////////////////////////////////////////////////////////    
//--------------TOR的状态设置
//设置状态
void TOR_Status_Set(TACTICS* tactics, TASK_STATUS status) {
    unsigned char current_status = ((*tactics) & STATUS_MASK) >> 6;
    if (current_status <= status) {
        for (unsigned i = 0; i < status - current_status; i++) {
            (*tactics) += STATUS_BASIS_CODE;
        }
    }
    else {
        for (unsigned i = 0; i < current_status - status; i++) {
            (*tactics) -= STATUS_BASIS_CODE;
        }
    }
}
//获取状态
TASK_STATUS TOR_Status_Get(TACTICS* tactics) {
    return (TASK_STATUS)(((*tactics) & STATUS_MASK) >> 6);
}

///////////////////////////////////////////////////////////结束//////////////////////////////////////////////////////////    

/*************************
* 哈希表操作
**************************/
///////////////////////////////////////////////////////////开始//////////////////////////////////////////////////////////    
//哈希表初始化
void Hash_Init(void) {
    for (unsigned short i = 0; i < HASH_TABLE_SIZE; i++) {
        Hash_TOR.array[i] = NULL;//所有的值都为空
    }
    Hash_TOR.size = 0;//总数为0；

}
//哈希函数设计(生成键值对)
unsigned int HashFunction(char* Task_Name) {
    unsigned long hash = 5381;
    int c = *Task_Name++;
    while (c) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
		c = *Task_Name++;
    }
    return hash % HASH_TABLE_SIZE;
}
//哈希表插入操作
int HashTable_Insert(TASK_ORBIT_RING* TOR) {
    // 通过哈希函数获取索引
    unsigned int index = HashFunction(TOR->Task_name);
    // 创建一个新的HashNode
#if HEAP_SCHEME == HEAP_SOLUTION_1  
    HashNode* newNode = (HashNode*)hj_malloc(sizeof(HashNode));
#endif

    if (!newNode) {
        return -1;//空间申请失败
    }
    newNode->TOR = TOR;
    newNode->next = NULL;

    // 插入到哈希表中，处理冲突，前向插入数据
    if (Hash_TOR.array[index] != NULL) {
        newNode->next = Hash_TOR.array[index];
    }
    Hash_TOR.array[index] = newNode;
    //更新hash数量
    Hash_TOR.size++;
    return 0;
}
//哈希表删除操作
int HashTable_Delete(char* Task_Name) {
    // 通过哈希函数获取索引
    unsigned int index = HashFunction(Task_Name);
    //定义两个临时变量
    HashNode* current = Hash_TOR.array[index];
    HashNode* prev = NULL;
    //遍历hash节点
    while (current != NULL) {
        if (strcmp(current->TOR->Task_name, Task_Name) == 0) {
            if (prev == NULL) { // 删除的是头节点
                Hash_TOR.array[index] = current->next;
            }
            else { // 删除的是中间或尾节点
                prev->next = current->next;
            }
#if HEAP_SCHEME == HEAP_SOLUTION_1  
            hj_free(current, sizeof(HashNode));
#endif
            Hash_TOR.size--;
            return 1; // 删除成功
        }
        prev = current;
        current = current->next;
    }
    return 0; // 未找到对应节点
}
//哈希表查询操作
TASK_ORBIT_RING* HashTable_Find(char* Task_Name) {
    // 通过哈希函数获取索引
    unsigned int index = HashFunction(Task_Name);
    //定义临时变量获取当前hash节点
    HashNode* current = Hash_TOR.array[index];
    //遍历节点
    while (current != NULL) {
        if (strcmp(current->TOR->Task_name, Task_Name) == 0) {
            return current->TOR;
        }
        current = current->next;
    }
    return NULL; // 未找到
}
///////////////////////////////////////////////////////////结束//////////////////////////////////////////////////////////    
/*************************
* 优先级调度链操作
**************************/
///////////////////////////////////////////////////////////开始//////////////////////////////////////////////////////////    
//优先级调度链操作
//调度链初始化
int SPC_Init(void) {
#if HEAP_SCHEME == HEAP_SOLUTION_1  
    SPC_Head = hj_malloc(sizeof(SCHEDULING_PRIORITY_CHAIN));
#endif

    if (!SPC_Head)//空间申请失败
        return -1;
    SPC_Head->Task_Ring = NULL;
    SPC_Head->Ring_Priority = 0;
    SPC_Head->Ring_Task_Amount = 0;	//SPC节点总数
    SPC_Head->Ring_Active_Amount = 0;//特殊优先级活跃数量
    SPC_Head->SPC_Type = SPC_TYPE_HEADER;
	SPC_Head->TOR_Leader = NULL;//特殊优先级TOR首
    SPC_Head->previous = NULL;	//SPC链末尾
    SPC_Head->next = NULL;		//SPC链链首
    return 0;
}
//添加
int SPC_Add(unsigned char Priority) {
    //定义一个探针用于查询链表
    SCHEDULING_PRIORITY_CHAIN* SPC_Probe = SPC_Head;

    //从头节点开始遍历链表来找到正确的插入位置
    while (SPC_Probe->next != NULL && SPC_Probe->next->Ring_Priority >= Priority) {
        if (SPC_Probe->next->Ring_Priority == Priority)//已经存在该优先级节点
            return 1;
        SPC_Probe = SPC_Probe->next;
    }
    //创建新节点
#if HEAP_SCHEME == HEAP_SOLUTION_1  
    SCHEDULING_PRIORITY_CHAIN* node = hj_malloc(sizeof(SCHEDULING_PRIORITY_CHAIN));
#endif

    if (!node) //空间申请失败
        return -1;

    //初始化新节点
    node->Task_Ring = NULL;
    node->Ring_Priority = Priority;
    node->Ring_Task_Amount = 0;
    node->Ring_Active_Amount = 0;
    node->SPC_Type = SPC_TYPE_GENERAL;
    node->TOR_Leader = NULL;
    node->previous = NULL;
    node->next = NULL;

    //尾节点存入头结点的信息中

    //当前SPC_Probe是要插入位置的前一个节点
    node->next = SPC_Probe->next;
    if (SPC_Probe->next != NULL) {//判断探针的下一个节点是否为空
        SPC_Probe->next->previous = node;//不为空节点就更新该节点的“前节点指针”
    }
    SPC_Probe->next = node;
    node->previous = SPC_Probe;
    //头结点总数自增
    SPC_Head->Ring_Task_Amount++;

    //尾节点存入头结点的信息中
    SCHEDULING_PRIORITY_CHAIN* temp = SPC_Head->previous;
    if (SPC_Head->previous == NULL) {
        //首次更新节点
        temp = SPC_Head->next;
    }
    while (temp->next != NULL) {
        temp = temp->next;
    }
    SPC_Head->previous = temp;

    return 0;
}
//删除
int SPC_Delete(unsigned char Priority) {
    //定义一个探针用于查询链表
    SCHEDULING_PRIORITY_CHAIN* SPC_Probe = SPC_Head->next;
    //遍历SPC链
    while (SPC_Probe != NULL) {
        if (SPC_Probe->Ring_Priority == Priority) {//定位到对应的节点
            //释放堆空间
            if (SPC_Probe->Ring_Task_Amount > 0) {//节点挂载了任务轨道环
                TASK_ORBIT_RING* temp = SPC_Probe->Task_Ring;//定义一个临时变量用于释放空间
                //断开循环链表
                temp->previous->next = NULL;
                while (SPC_Probe->Task_Ring != NULL) {
                    temp = temp->next;
                    #if HEAP_SCHEME == HEAP_SOLUTION_1  
                        //释放堆栈空间
                        hj_free(SPC_Probe->Task_Ring->TCB.stack_bottom, SPC_Probe->Task_Ring->TCB.stack_size);
                        //释放节点
                        hj_free(SPC_Probe->Task_Ring, sizeof(TASK_ORBIT_RING));
                    #endif
                    //减少SPC中记录的数量
                    SPC_Probe->Ring_Task_Amount--;
                    //更新节点
                    SPC_Probe->Task_Ring = temp;
                }
            }
            //删除调度链的节点
            //如果链表的探头刚好指向了即将释放的节点
            if (SPC_Detector == SPC_Probe) {
                //情况1：链表只有一个节点，那么探头回到头结点（移动到上一个节点）
                //情况2：链表有多个节点，但是被删除的节点是优先级最高的节点，那么探头移动到下一个节点
                //情况3：链表有多个节点，被删除的是优先级最低的节点，那么探头移动到上一个节点
                //情况4：链表有多个节点，被删除的节点是中间节点，那么探头移动到上一个节点
                if (SPC_Detector->previous == SPC_Head && SPC_Head->Ring_Task_Amount > 1) {//情况2
                    SPC_Detector = SPC_Detector->next;//探头指向下一个节点
                }
                else {
                    SPC_Detector = SPC_Detector->previous;//探头上移；
                }

            }
            //删除探针指向的节点
            if (SPC_Probe->next != NULL) {//探针下一个节点不为空
                SPC_Probe->next->previous = SPC_Probe->previous;
            }
            SPC_Probe->previous->next = SPC_Probe->next;
            //释放节点
            #if HEAP_SCHEME == HEAP_SOLUTION_1  
                hj_free(SPC_Probe, sizeof(SCHEDULING_PRIORITY_CHAIN));
            #endif

            //调度链总数自减
            --SPC_Head->Ring_Task_Amount;
            return 0;
        }
        SPC_Probe = SPC_Probe->next;
    }
    return -1;//SPC链中不存在该优先级节点
}
//获取SPC_Head的值
SCHEDULING_PRIORITY_CHAIN* Get_SPC_Head() {
    return SPC_Head;
}
//获取SPC_Detector的值
SCHEDULING_PRIORITY_CHAIN* Get_SPC_Detector() {
    return SPC_Detector;
}
//获取指定优先级的调度链节点
SCHEDULING_PRIORITY_CHAIN* Get_Designated_SPC(unsigned char Priority) {
    SCHEDULING_PRIORITY_CHAIN* temp = SPC_Head->next;
    while (temp != NULL) {
        if (temp->Ring_Priority == Priority)
            return temp;
        temp = temp->next;
    }
    return NULL;
}
//查询是否存在指定优先级的调度链节点
int SPC_Verify(unsigned char Priority) {
    if (!SPC_Head->next) {//调度链为空
        return -1;
    }
    SCHEDULING_PRIORITY_CHAIN* temp = SPC_Head->next;
    while (temp != NULL) {
        if (temp->Ring_Priority == Priority)
            return 1;//存在
        temp = temp->next;
    }
    return 0;//不存在
}

///////////////////////////////////////////////////////////结束//////////////////////////////////////////////////////////


/*************************
* 任务轨道环操作
**************************/
//任务轨道环操作
///////////////////////////////////////////////////////////开始//////////////////////////////////////////////////////////

//添加一个TOR节点(!!!!!!!!!!!注意，只是添加了一个TOR，并没有在哈希表中添加节点，目前规划在未来的Task_Create中进行两者结合)
int TOR_Create(TASK_FUNCTION taskFunction, \
    void* Args, \
    unsigned char Priority, \
    unsigned char* icon, \
    char* Task_name, \
    StackDepth size, \
    TASK_ORBIT_RING** node) {
    //检测优先级
    if (Priority > 15)
        Priority = 15;
    //添加一个指定优先级的SPC
    if (SPC_Add(Priority) == -1) {
        return -1;//对应SPC启用失败
    }
    //获取指定SPC节点
    SCHEDULING_PRIORITY_CHAIN* temp = Get_Designated_SPC(Priority);


    //申请TOR节点
    #if HEAP_SCHEME == HEAP_SOLUTION_1  
        (*node) = hj_malloc(sizeof(TASK_ORBIT_RING));
    #endif


    if (!(*node)) {
        return -2;//TOR申请失败
    }
    //TOR初始化——————————————————————————开始
    //TCB初始化（不申请空间，只设初始）
    (*node)->TCB.taskFunction = taskFunction;
    (*node)->TCB.Args = Args;
    (*node)->TCB.stack_bottom = NULL;
    (*node)->TCB.stack_size = size;
    (*node)->TCB.stack_top = NULL;
    //TOR其他参数
    (*node)->self_SPC = temp;//保存SPC
    (*node)->Task_Enable_Flag = TOR_DISABLE_FLAG;//不启用
    (*node)->icon = icon;
    (*node)->self_handle = (*node);
    (*node)->tactics = Priority;//其他字段为0，优先级字段赋值
    (*node)->next = NULL;
    (*node)->previous = NULL;
    //任务名称初始化

    strcpy((*node)->Task_name, Task_name);
    //TOR初始化-----------------------------------------------------结束

    //TOR入环
    //情况一：当前SPC没有挂载任务
    if (temp->Ring_Task_Amount == 0) {
        temp->Task_Ring = (*node);
        temp->Task_Ring->next = (*node);
        temp->Task_Ring->previous = (*node);
        //更新SPC中的TOR环首标记
        temp->TOR_Leader = (*node);
    }
    else {
        //情况二：SPC挂载了任务.此时TOR加入到SPC挂载点的尾部插入
        //TOR附着在环上
        (*node)->next = temp->TOR_Leader;
        (*node)->previous = temp->TOR_Leader->previous;
        //插入环中
        (*node)->next->previous = (*node);
        (*node)->previous->next = (*node);
    }
    ++temp->Ring_Task_Amount;//增加节点总数
    return 0;
}

//TOR移动（优先级出现改变后，从当前SPC移动到指定优先级的SPC）
int TOR_Move(TASK_ORBIT_RING* TOR) {
    //检测节点是否为空
    if (!TOR) {
        return -1;//如果为空返回错误
    }
    //情况1：此SPC还存在节点
    //确认目标优先级的SPC是否存在a(存在或者创建成功则会继续执行)
    if (SPC_Add(TOR_Priority_Get(TOR->self_handle)) == -1) {
        //创建失败
        return -1;
    }
    //剥离目标TOR
    TOR->next->previous = TOR->previous;
    TOR->previous->next = TOR->next;
    //获取目标SPC句柄
    SCHEDULING_PRIORITY_CHAIN* temp = Get_Designated_SPC(TOR_Priority_Get(TOR->self_handle));
    if (temp->Ring_Task_Amount == 0) {
        //如果是新开辟的SPC节点
        temp->Ring_Task_Amount++;
        temp->TOR_Leader = TOR;
        temp->Task_Ring = TOR;
        TOR->next = TOR;
        TOR->previous = TOR;
        return 0;
    }
    //TOR附着在TOR环上
    TOR->next = temp->TOR_Leader;
    TOR->previous = temp->TOR_Leader->previous;
    //入环
    TOR->next->previous = TOR;
    TOR->previous->next = TOR;
    temp->Ring_Task_Amount++;
    return 0;
}

//---------------TOR启动与禁用
//启用指定TOR
int TOR_Enable(TASK_ORBIT_RING* TOR) {
    if (!TOR) {
        return -1;
    }
    //验证这个TOR是否已经启用
    if (TOR->Task_Enable_Flag == TOR_ENABLE_FLAG) {
        //如果该TOR已经启用则直接返回
        return 0;
    }
    #if HEAP_SCHEME == HEAP_SOLUTION_1  
        TOR->TCB.stack_bottom = (stack_t*)hj_malloc(TOR->TCB.stack_size*sizeof(stack_t));//设置栈底部，开辟空间
    #endif
    if (!TOR->TCB.stack_bottom) {
        return -1;//堆栈空间申请失败
    }
    TOR->TCB.stack_top = TOR->TCB.stack_bottom + TOR->TCB.stack_size;//设置栈顶
    *TOR->TCB.stack_bottom = STACK_DETECTION_VALUE;//栈底设置一个特定值
    TOR->Task_Enable_Flag = TOR_ENABLE_FLAG;
    return 0;
}
//禁用指定TOR
int TOR_Disable(TASK_ORBIT_RING* TOR) {
    /*申请结束任务的过程，应当保证没有是原子性的。
   *当堆栈被释放后，如果内核正常调度，那么该任务的任务帧会由于压栈出现异常导致系统崩溃。
   */
    if (!TOR) {
        return -1;
    }
    //关闭任务全局中断(！！！！！！！！以下代码在移植后开启)
    __disable_irq(); // 关闭全局中断

    //验证这个TOR是否启用
    if (TOR->Task_Enable_Flag == TOR_DISABLE_FLAG) {
        //如果没有启用则直接返回
        //开启任务全局中断（！！！！！！！！以下代码在移植后开启）
    __enable_irq(); // 开启全局中断
        return 0;
    }
    #if HEAP_SCHEME == HEAP_SOLUTION_1  
        hj_free(TOR->TCB.stack_bottom, TOR->TCB.stack_size*sizeof(stack_t));//释放指定TOR的堆栈空间
    #endif
    TOR->TCB.stack_bottom = NULL;
    TOR->TCB.stack_top = NULL;
    TOR->Task_Enable_Flag = TOR_DISABLE_FLAG;//关闭启用标志
    //如果当前的状态处于活跃态，那么将SPC的活跃态数量减少一个
    if (TOR_Status_Get(&TOR->tactics) != TASK_OBSTRUCT) {
        TOR->self_SPC->Ring_Active_Amount--;
    }
    //开启任务全局中断（！！！！！！！！以下代码在移植后开启）
    __enable_irq(); // 开启全局中断
    return 0;
}
//堆栈溢出检测
Stack_Overflow_t TOR_Stack_Overflow_Detection(TASK_ORBIT_RING* TOR) {
    int detection_result = TOR->TCB.stack_top - TOR->TCB.stack_bottom;
    //检测栈底默认值是否改变
    if (*TOR->TCB.stack_bottom != STACK_DETECTION_VALUE){
        detection_result = -1;
	}
    //判断返回    
    if (detection_result < 0) {
        return (Stack_Overflow_t)-1;//栈溢出
    }
    else if (detection_result == 0) {
        return (Stack_Overflow_t)1;//溢出临界点
    }
    else {
        return (Stack_Overflow_t)0;//安全
    }
}
//返回优先级最低且已经启用的一个TOR
TASK_ORBIT_RING* TOR_Lowest_Enable_Find() {
    //定义一个临时SPC指针
    SCHEDULING_PRIORITY_CHAIN* temp_spc = SPC_Head->previous;
    if (!temp_spc) {
        return NULL;//SPC链为空
    }
    //开始逆向遍历SPC链
    while (temp_spc != SPC_Head) {
        //定义一个临时TOR指针
        TASK_ORBIT_RING* temp_tor = temp_spc->TOR_Leader;
        //进入TOR环寻找挂起的任务
        do {
            //验证是否出现了已经启用的任务
            if (temp_tor->Task_Enable_Flag == TOR_ENABLE_FLAG) {
                //找到符合要求的任务,
                return temp_tor;
            }
            temp_tor = temp_tor->next;
        } while (temp_tor != temp_spc->TOR_Leader);
    }
    return NULL;//异常，SPC链不存在启用的任务（通常不可能）
}
//返回优先级最低且已经启用且处于指定状态的TOR
TASK_ORBIT_RING* TOR_Lowest_Enable_Designated_Status_Find(TASK_STATUS status){
    //定义一个临时SPC指针
    SCHEDULING_PRIORITY_CHAIN* temp_spc = SPC_Head->previous;
    if (!temp_spc) {
        return NULL;//SPC链为空
    }
    TASK_ORBIT_RING* temp_tor = NULL;
	
	//如果要找的节点是挂起态
	if(status == TASK_OBSTRUCT){
		//如果要找的节点是挂起态
		while(temp_spc != SPC_Head){
			//定义一个临时TOR指针
			temp_tor = temp_spc->TOR_Leader;
			do{
				if(temp_tor->Task_Enable_Flag == TOR_ENABLE_FLAG && TOR_Status_Get(&temp_tor->tactics) == TASK_OBSTRUCT){
					return temp_tor;
				}
				temp_tor = temp_tor->next;
			} while (temp_tor != temp_spc->TOR_Leader);
			temp_spc = temp_spc->previous;
		}
		//跳出循环代表SPC链所有任务都没有启动，故返回错误值
	}else if(status == TASK_READY){
		//如果要找的节点是就绪态
		//开始逆向遍历SPC链
		while(temp_spc != SPC_Head && temp_spc->Ring_Active_Amount == 0 ){
			temp_spc = temp_spc->previous;
		}
		//如果不存在具有活跃的SPC节点，返回错误
		if(temp_spc == SPC_Head){
			return NULL;
		}
		//定义一个临时TOR指针
		temp_tor = temp_spc->TOR_Leader;
		
		do{
			if(temp_tor->Task_Enable_Flag == TOR_ENABLE_FLAG && TOR_Status_Get(&temp_tor->tactics) == TASK_READY){
				return temp_tor;
			}
			temp_tor = temp_tor->next;
		} while (temp_tor != temp_spc->TOR_Leader);
			
	}		
    return NULL;//异常，SPC链不存在指定要求的任务（通常不可能）
}
///////////////////////////////////////////////////////////结束//////////////////////////////////////////////////////////

/******************
* 任务切换
*******************/
///////////////////////////////////////////////////////////开始//////////////////////////////////////////////////////////

//在当前环中寻找可执行的任务
/*
* 1、在当前环有可用任务的情况下：
* （1）当前环处于具有活跃任务环的最高优先级SPC节点，则循环遍历当前TOR环的节点
* （2）若比当前环更高优先级的SPC出现了任务唤醒或者任务启用的情况下，跳转到该SPC节点
* 2、在当前环不存在可用任务的情况下：
* （1）若比当前环更高优先级的SPC出现了任务唤醒或者任务启用的情况下，跳转到该SPC节点
* （2）如果不存在更高优先级的SPC出现活跃态任务，那么往下遍历，寻找可执行的SPC节点
* （3）SPC链不存在可执行任务时，执行空闲任务
* -------------
* 目前的想法是设置一个标志位，如果标志位改变就检查SPC，否则执行当前SPC。
*/

//根据策略，选择两种切换方式
#if SPECIAL_PRIORITY_CONFIG == SPECIAL_PRIORITY_DISABLE
void Task_Switch(void) {
    //【总情况1】任务首次执行，此时探头指向NULL（仅且只有一次指向NULL的可能，之后在不出现异常的情况下只可能在Task_Idle和普通任务之间切换）
    if (SPC_Detector == NULL) {
        //让探头指向第一个SPC
        SPC_Detector = SPC_Head->next;
        if (SPC_Detector == NULL) {
            //执行空闲任务代码
            /*
            * 此时如果首个SPC不存在，那么此时系统出现异常，
            * 为了应对异常情况，让SPC执行空闲任务
            */
            SPC_Detector = &SPC_Idle;
            return;
        }
        //遍历SPC链寻找合适的SPC,当探头没有遍历到SPC链尾的条件下，只要遇到了不存在活跃节点的SPC时，就立刻往后遍历
        while (SPC_Detector != NULL && SPC_Detector->Ring_Active_Amount == 0) {
            SPC_Detector = SPC_Detector->next;
        }
        //遍历结果1：
        if (SPC_Detector == NULL) {
            /*
            * 此时的探头处于链尾，也就意味着SPC链不存在可以执行的任务，
            * 为了应对异常，应当执行空闲任务
            */
            //空闲任务代码
            SPC_Detector = &SPC_Idle;
            return;
        }
        /*遍历结果2，是找到了合适的任务
        * 将该任务设置为运行态并启动
        */
		//过滤掉没有启用的任务和处于挂起态的任务
		while(SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT){
			SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
		}
        //选中任务设置为运行态
        TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_RUNTIME);
        return;
    }
	
	//栈溢出检测(存在上一个任务被终止的可能，被终止后再计算会产生未定义行为，所以筛选出来处于启用状态的TOR)
	if(SPC_Detector->Task_Ring->Task_Enable_Flag){
		STACK_OVERFLOW_FLAG = TOR_Stack_Overflow_Detection(SPC_Detector->Task_Ring);
	}
	
	
	/*【更新任务等待队列】
	*将所有的定时阻塞都进行时间自减，如果时间为0，则立刻唤醒该任务
	*/
	Synchronization_Primitives_Update();
    //还存在一种设想【总情况3】任务处于空闲任务时进行任务切换

    //如果当前任务是执行的空闲任务
    if (SPC_Detector == &SPC_Idle) {
        //情况1：存在新任务被唤醒或者出现
        if (TaskQueueUpdated == NEW_TASK_AVAILABLE) {
            SPC_Detector = SPC_Head->next;//将当前探头更新为首个SPC
            //遍历整个SPC链，寻找新任务
            while (SPC_Detector != NULL && SPC_Detector->Ring_Active_Amount == 0) {
                SPC_Detector = SPC_Detector->next;
            }
            //此时的SPC_Detector为新任务的SPC节点
			if(SPC_Detector == NULL){
				//如果发现没有新任务，那么重新进入空闲任务
				//新任务标志重置
				TaskQueueUpdated = NEW_TASK_NOT_AVAILABLE;
				SPC_Detector = &SPC_Idle;
				return;
			}
			//寻找到可执行的TOR节点			
			while(SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT){
				SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
			}
            //新任务标志重置
            TaskQueueUpdated = NEW_TASK_NOT_AVAILABLE;
        }
		return;
    }



    // 【总情况2】：非首次执行任务切换
    // 
    //检查当前任务是否被设置为了挂起态
    if (TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) != TASK_OBSTRUCT) {
        //如果当前任务是活跃态，那么就可以设置当前任务设置为就绪态
        TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_READY);
    }

    /*检查新任务标志位是否被改变
    * 【1】如果标志位显示没有新任务加入：
    * （1）如果当前环存在可用任务，则在当前环进行遍历
    * （2）如果当前环不存在可用任务，那么探头朝着尾部的方向遍历
    * 【2】如果标志位显示有新任务加入：立刻用一个临时指针朝着SPC头结点的方向遍历
    * （1）如果发现了新任务在高优先级SPC，那么立刻将探头移动到该优先级SPC
    * （2）如果遍历到了头结点也没有发现活跃任务，那么立刻回到当前的SPC节点按照情况【1】开始进行
    */
    
    if (TaskQueueUpdated == NEW_TASK_NOT_AVAILABLE) {
    not_available:;//这个跳转标签为情况【2】中的跳转标志
        //新任务标志位没有出现改变
         //判断当前TOR环中是否还有活跃的TOR节点
        if (SPC_Detector->Ring_Active_Amount == 0) {
            //当前TOR环不存在可运行的节点
            
            //寻找一个可以具有活跃TOR的SPC
            do {
                SPC_Detector = SPC_Detector->next;
            } while (SPC_Detector != NULL && SPC_Detector->Ring_Active_Amount == 0);


            /*
            * 判断返回的结果，情况有两种：
            * 【1】返回一个SPC，寻找到了符合要求的SPC
            * 【2】返回NULL，没有可执行的任务。
            */

            if (!SPC_Detector) {
                //情况【1】如果返回结果为空，
                /*
                * 空闲任务代码...
                */
                SPC_Detector = &SPC_Idle;
                return;
            }
            //情况【2】正常的SPC,由于这个SPC中一定有至少一个可用的TOR，所以不会陷入死循环
            while (SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT) {
                SPC_Detector = SPC_Detector->next;
            }
            //遍历完成后，此时的SPC_Detector为接下来要执行的任务
        }
        else {//当前环存在可运行节点
            //SPC的探针前移动,如果检测到未启用的任务或者被挂起的任务则继续遍历，直到找到一个有效的任务
            do {
                SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
            } while (SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT);
        }
    }
    else {
        //重新将标志位设置为“没有新任务”
        TaskQueueUpdated = NEW_TASK_NOT_AVAILABLE;
        //新任务标志位出现了改变
        SCHEDULING_PRIORITY_CHAIN* temp_task_detector = SPC_Detector->previous;//定义一个临时SPC探头往前遍历
        while (temp_task_detector != SPC_Head) {
            //如果找到一个符合要求的SPC立刻退出
            if (temp_task_detector->Ring_Active_Amount > 0) {
                break;
            }
            temp_task_detector = temp_task_detector->previous;
        }
        //检查遍历结果
        /*
        * 【1】找到了符合要求的SPC，
        * 【2】遍历至头结点，也就是前向的SPC链没有TOR激活
        */
        if (temp_task_detector == SPC_Head) {
            //如果此时的SPC前向链没有存在活跃的TOR，那么跳转会原来的SPC并按照情况【1】进行
            goto not_available;
        }

        //找到了符合要求的节点，将临时探头的值赋值给探头
        SPC_Detector = temp_task_detector;

        //情况【2】正常的SPC,由于这个SPC中一定有至少一个可用的TOR，所以不会陷入死循环
        while (SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT) {
            SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
        }
        //遍历完成后，此时的SPC_Detector为接下来要执行的任务


    }
    //选中任务设置为运行态
    TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_RUNTIME);
	return;
}
#else
//unsigned char Special_Priority_Count = 0;//定义一个变量记录特殊优先级的TOR总数，因为SPC_Head的这个成员被用来记录SPC的总数了
SCHEDULING_PRIORITY_CHAIN* SPC_Standard = NULL;//定一个变量记录标准的优先级探头的位置
//根据当前值判断应该执行特殊优先级还是标准优先级
unsigned char Priority_Switch_Tactics = 0;//标准优先级为0，特殊优先级为1
void Task_Switch(void){
	 //【总情况1】任务首次执行，此时探头指向NULL（仅且只有一次指向NULL的可能，之后在不出现异常的情况下只可能在Task_Idle和普通任务之间切换）
    if (SPC_Detector == NULL) {
        //让探头指向第一个SPC
        SPC_Detector = SPC_Head->next;
        if (SPC_Detector == NULL) {
            //执行空闲任务代码
            /*
            * 此时如果首个SPC不存在，那么此时系统出现异常，
            * 为了应对异常情况，让SPC执行空闲任务
            */
            SPC_Detector = &SPC_Idle;
			//记录当前的SPC位置
			SPC_Standard = SPC_Detector;
			//切换执行策略
			Priority_Switch_Tactics = 1;
            return;
        }
        //遍历SPC链寻找合适的SPC,当探头没有遍历到SPC链尾的条件下，只要遇到了不存在活跃节点的SPC时，就立刻往后遍历
        while (SPC_Detector != NULL && SPC_Detector->Ring_Active_Amount == 0) {
            SPC_Detector = SPC_Detector->next;
        }
        //遍历结果1：
        if (SPC_Detector == NULL) {
            /*
            * 此时的探头处于链尾，也就意味着SPC链不存在可以执行的任务，
            * 为了应对异常，应当执行空闲任务
            */
            //空闲任务代码
            SPC_Detector = &SPC_Idle;
			//记录当前的SPC位置
			SPC_Standard = SPC_Detector;
			//切换执行策略
			Priority_Switch_Tactics = 1;
            return;
        }
        /*遍历结果2，是找到了合适的任务
        * 将该任务设置为运行态并启动
        */
		//过滤掉没有启用的任务和处于挂起态的任务
		while(SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT){
			SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
		}
		
        //选中任务设置为运行态
        TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_RUNTIME);
		//记录当前的SPC位置
		SPC_Standard = SPC_Detector;
		//切换执行策略
		Priority_Switch_Tactics = 1;
        return;
    }
	
	//栈溢出检测(存在上一个任务被终止的可能，被终止后再计算会产生未定义行为，所以筛选出来处于启用状态的TOR)
	if(SPC_Detector->Task_Ring->Task_Enable_Flag){
		STACK_OVERFLOW_FLAG = TOR_Stack_Overflow_Detection(SPC_Detector->Task_Ring);
	}
	
	
	/*【更新任务等待队列】
	*将所有的定时阻塞都进行时间自减，如果时间为0，则立刻唤醒该任务
	*/
	Synchronization_Primitives_Update();
    //还存在一种设想【总情况3】任务处于空闲任务时进行任务切换

	tactics_select:;//策略选择跳转标志
	
	//如果当前策略是执行标准优先级------------------------
	if(Priority_Switch_Tactics == 0){
		SPC_Detector = SPC_Standard;//将之前记录的标准优先级位置赋值给探头
		//如果当前任务是执行的空闲任务
		if (SPC_Detector == &SPC_Idle) {
			//情况1：存在新任务被唤醒或者出现
			if (TaskQueueUpdated == NEW_TASK_AVAILABLE) {
				SPC_Detector = SPC_Head->next;//将当前探头更新为首个SPC
				//遍历整个SPC链，寻找新任务
				while (SPC_Detector!=NULL && SPC_Detector->Ring_Active_Amount == 0) {
					SPC_Detector = SPC_Detector->next;
				}
				//此时的SPC_Detector为新任务的SPC节点
				//如果此时的SPC没有节点，那么重新使用空闲任务
				if(SPC_Detector == NULL){
					//新任务标志重置
					TaskQueueUpdated = NEW_TASK_NOT_AVAILABLE;
					SPC_Detector = &SPC_Idle;
					//记录当前的SPC位置
					SPC_Standard = SPC_Detector;
					//切换执行策略
					Priority_Switch_Tactics = 1;
					return;
				}
				//寻找到可执行的TOR节点			
				while(SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT){
					SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
				}
				//新任务标志重置
				TaskQueueUpdated = NEW_TASK_NOT_AVAILABLE;
			}
			//记录当前的SPC位置
			SPC_Standard = SPC_Detector;
			//切换执行策略
			Priority_Switch_Tactics = 1;
			return;
		}



		// 【总情况2】：非首次执行任务切换
		// 
		//检查当前任务是否被设置为了挂起态
		if (TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) != TASK_OBSTRUCT) {
			//如果当前任务是活跃态，那么就可以设置当前任务设置为就绪态
			TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_READY);
		}

    /*检查新任务标志位是否被改变
    * 【1】如果标志位显示没有新任务加入：
    * （1）如果当前环存在可用任务，则在当前环进行遍历
    * （2）如果当前环不存在可用任务，那么探头朝着尾部的方向遍历
    * 【2】如果标志位显示有新任务加入：立刻用一个临时指针朝着SPC头结点的方向遍历
    * （1）如果发现了新任务在高优先级SPC，那么立刻将探头移动到该优先级SPC
    * （2）如果遍历到了头结点也没有发现活跃任务，那么立刻回到当前的SPC节点按照情况【1】开始进行
    */
    
		if (TaskQueueUpdated == NEW_TASK_NOT_AVAILABLE) {
		not_available:;//这个跳转标签为情况【2】中的跳转标志
			//新任务标志位没有出现改变
			 //判断当前TOR环中是否还有活跃的TOR节点
			if (SPC_Detector->Ring_Active_Amount == 0) {
				//当前TOR环不存在可运行的节点
				
				//寻找一个可以具有活跃TOR的SPC
				do {
					SPC_Detector = SPC_Detector->next;
				} while (SPC_Detector != NULL && SPC_Detector->Ring_Active_Amount == 0);


				/*
				* 判断返回的结果，情况有两种：
				* 【1】返回一个SPC，寻找到了符合要求的SPC
				* 【2】返回NULL，没有可执行的任务。
				*/

				if (!SPC_Detector) {
					//情况【1】如果返回结果为空，
					/*
					* 空闲任务代码...
					*/
					SPC_Detector = &SPC_Idle;
					//记录当前的SPC位置
					SPC_Standard = SPC_Detector;
					//切换执行策略
					Priority_Switch_Tactics = 1;
					return;
				}
				//情况【2】正常的SPC,由于这个SPC中一定有至少一个可用的TOR，所以不会陷入死循环
				while (SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT) {
					SPC_Detector = SPC_Detector->next;
				}
				//遍历完成后，此时的SPC_Detector为接下来要执行的任务
			}
			else {//当前环存在可运行节点
				//SPC的探针前移动,如果检测到未启用的任务或者被挂起的任务则继续遍历，直到找到一个有效的任务
				do {
					SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
				} while (SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT);
			}
		}
		else {
			//重新将标志位设置为“没有新任务”
			TaskQueueUpdated = NEW_TASK_NOT_AVAILABLE;
			//新任务标志位出现了改变
			SCHEDULING_PRIORITY_CHAIN* temp_task_detector = SPC_Detector->previous;//定义一个临时SPC探头往前遍历
			while (temp_task_detector != SPC_Head) {
				//如果找到一个符合要求的SPC立刻退出
				if (temp_task_detector->Ring_Active_Amount > 0) {
					break;
				}
				temp_task_detector = temp_task_detector->previous;
			}
			//检查遍历结果
			/*
			* 【1】找到了符合要求的SPC，
			* 【2】遍历至头结点，也就是前向的SPC链没有TOR激活
			*/
			if (temp_task_detector == SPC_Head) {
				//如果此时的SPC前向链没有存在活跃的TOR，那么跳转会原来的SPC并按照情况【1】进行
				goto not_available;
			}

			//找到了符合要求的节点，将临时探头的值赋值给探头
			SPC_Detector = temp_task_detector;

			//情况【2】正常的SPC,由于这个SPC中一定有至少一个可用的TOR，所以不会陷入死循环
			while (SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT) {
				SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
			}
			//遍历完成后，此时的SPC_Detector为接下来要执行的任务
		}
		//选中任务设置为运行态
		TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_RUNTIME);
		//记录当前的SPC位置
		SPC_Standard = SPC_Detector;
		//切换执行策略
		Priority_Switch_Tactics = 1;
		return;
	}else{
		//如果当前是特殊优先级
		SPC_Detector = SPC_Head;//将形式头节点的值赋值给探头
		
		//检测形势头节点是否具有活跃的任务
		if(SPC_Detector->Ring_Active_Amount == 0){
			//如果头结点没有活跃的任务，则立刻跳转到标准优先级
			Priority_Switch_Tactics = 0;//更改执行策略
			goto tactics_select;
		}
		
		//过滤掉没有活跃任务的情况后
		//检查当前任务是否被设置为了挂起态
		if (TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) != TASK_OBSTRUCT) {
			//如果当前任务是活跃态，那么就可以设置当前任务设置为就绪态
			TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_READY);
		}
		
		//处理完成当前任务后，寻找下一个任务,由于检测了一定具有活跃任务，所以不会陷入死循环
		do{
			SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
			//只要目标处于不启动状态或者挂起态，立刻继续遍历
		}while(SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT);
		
		//获取到新的符合要求的节点后,将其设置为进行态立刻返回
		//选中任务设置为运行态
		TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_RUNTIME);
		//修改优先级策略
		Priority_Switch_Tactics = 0;
		return;
	}
}
#endif
/*************************
* 空闲任务
**************************/
//空闲任务
///////////////////////////////////////////////////////////开始//////////////////////////////////////////////////////////
//空闲任务函数
void Task_Idle(void* args) {
	while(1);
}
//空闲任务初始化
void Task_Idle_Init(void) {
    //SPC_Idle初始化
    SPC_Idle.Ring_Priority = 0;
    SPC_Idle.next = NULL;
    SPC_Idle.previous = NULL;
    SPC_Idle.Ring_Active_Amount = 1;
    SPC_Idle.Ring_Task_Amount = 1;
    SPC_Idle.SPC_Type = SPC_TYPE_GENERAL;
    SPC_Idle.Task_Ring = &TOR_Idle;
    SPC_Idle.TOR_Leader = &TOR_Idle;
    //TOR_Idle初始化
    TOR_Idle.icon = NULL;
    TOR_Idle.next = NULL;
    TOR_Idle.previous = NULL;
    TOR_Idle.self_handle = &TOR_Idle;
    TOR_Idle.self_SPC = &SPC_Idle;
    TOR_Idle.tactics = 0;
    TOR_Idle.Task_Enable_Flag = TOR_ENABLE_FLAG;
    strcpy(TOR_Idle.Task_name, "Task_Idle");
    //TCB定义
    TOR_Idle.TCB.Args = NULL;
    TOR_Idle.TCB.stack_bottom = &Task_Idle_Stack[0];
    TOR_Idle.TCB.stack_size = TASK_IDLE_STACK_SIZE;
    TOR_Idle.TCB.stack_top = &Task_Idle_Stack[TOR_Idle.TCB.stack_size];
    TOR_Idle.TCB.taskFunction = Task_Idle;
    //TCB初始化

    *(--TOR_Idle.TCB.stack_top) = (stack_t)0x01000000u;   							        // xPSR bit 24 = 1
    *(--TOR_Idle.TCB.stack_top) = (stack_t)TOR_Idle.TCB.taskFunction;    					// R15 PC 
    *(--TOR_Idle.TCB.stack_top) = (stack_t)0x00000014u; 							        // R14 LR
    *(--TOR_Idle.TCB.stack_top) = (stack_t)0x00000012u; 							        // R12
    *(--TOR_Idle.TCB.stack_top) = (stack_t)0x00000003u;							            // R3
    *(--TOR_Idle.TCB.stack_top) = (stack_t)0x00000002u;							            // R2
    *(--TOR_Idle.TCB.stack_top) = (stack_t)0x00000001u; 						        	// R1
    *(--TOR_Idle.TCB.stack_top) = (stack_t)TOR_Idle.TCB.Args; 					    		// R0
    *(--TOR_Idle.TCB.stack_top) = (stack_t)0x00000011u; 						        	// R11
    *(--TOR_Idle.TCB.stack_top) = (stack_t)0x00000010u;					            		// R10
    *(--TOR_Idle.TCB.stack_top) = (stack_t)0x00000009u; 				        			// R9
    *(--TOR_Idle.TCB.stack_top) = (stack_t)0x00000008u;				            			// R8
    *(--TOR_Idle.TCB.stack_top) = (stack_t)0x00000007u;				            			// R7
    *(--TOR_Idle.TCB.stack_top) = (stack_t)0x00000006u; 			        				// R6
    *(--TOR_Idle.TCB.stack_top) = (stack_t)0x00000005u; 			        				// R5
    *(--TOR_Idle.TCB.stack_top) = (stack_t)0x00000004u;				            			// R4
}

///////////////////////////////////////////////////////////结束//////////////////////////////////////////////////////////


//时钟中断执行ISR
void SysTick_Handler(void) {
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}


/*************************
* 任务API接口
*************************/
/**任务内核配置**/
//-----------------------------------------------------------任务内核配置 开始------------------------------------------------

//时间片配置
//系统滴答定时器初始化【定时中断，功能正常，测试通过】
void SysTick_init(void) {
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;//定时器配置（默认不分频）
	SysTick->VAL   = 0;
	SysTick->LOAD  = SYSTICK_LOAD;//系统重装值。
	NVIC_SetPriority(SysTick_IRQn, 0x02);
}

//任务内核初始化
int Inkos_Init(void){
	if(SPC_Init()){
		return -1;
	}
	Hash_Init(); 
    Inspector_Init();
	SysTick_init();
	Task_Idle_Init();
	return 0;
}

//操作系统启动
void Inkos_StartUp(void){
	Task_Switch();
	__asm {//用户堆栈指针初始化
		MOV R0, 0x0
		MSR PSP, R0
	}
	Open_Task_Scheduler();
}

//-----------------------------------------------------------任务内核配置 结束------------------------------------------------
//-----------------------------------------------------------任务的创建和加载 开始------------------------------------------------
/*
* 1、创建TOR，插入SPC中
* 2、加入hash表中
* 3、检测用户名的合法性
*/
int Task_Create(TASK_FUNCTION taskFunction, \
    void* Args, \
    unsigned char Priority, \
    unsigned char* icon, \
    char* Task_name, \
    StackDepth size) {
    //创建一个TOR指针;
    TASK_ORBIT_RING* node = NULL;
    //申请创建任务
    // 任务名称规范检测
    //字符串的TASK_NAME_SIZE+1位补\0;
    //任务名称长度
    char task_name_temp[TASK_NAME_SIZE + 1];
    task_name_temp[TASK_NAME_SIZE] = '\0';
    if (strlen(Task_name) <= TASK_NAME_SIZE) {
        for (unsigned char i = 0; i < strlen(Task_name) + 1; i++) {
            task_name_temp[i] = Task_name[i];
        }
    }
    else {
        return -1;//命名过长
    }
    //任务名是否存在重复
    if (HashTable_Find(task_name_temp)) {
        return -4;//任务名已经存在
    }

    int value = TOR_Create(taskFunction, Args, Priority, icon, task_name_temp, size, &node);
    if (value) {//任务创建
        return value;
    }
    //插入hash表
    if (HashTable_Insert(node->self_handle)) {
        //插入失败
    #if HEAP_SCHEME == HEAP_SOLUTION_1  
        hj_free(node, sizeof(TASK_ORBIT_RING));
    #endif

        return -1;
    }
    return 0;
}

//计划将此函数设置为原子函数，以防初始化的过程中被打断
int Task_Load(TASK_ORBIT_RING* TOR) {
    if (!TOR) {
        return -1;//参数异常
    }
    //检测任务是否已经被开启
    if (TOR->Task_Enable_Flag == TOR_ENABLE_FLAG) {
        //如果任务已经开启了则返回
        return 1;
    }
flag:;//经过处理之后，再次申请启动新任务
    //开启新任务
    if (TOR_Enable(TOR)) {
        //策略1
        #if TASK_LOAD_SOLUTION == TASK_LOAD_SOLUTION_1 
            return -1;//任务加载失败
        //策略2
        #elif TASK_LOAD_SOLUTION == TASK_LOAD_SOLUTION_2

            SCHEDULING_PRIORITY_CHAIN* temp_spc = SPC_Head;
     
            //查找当前具有活跃任务的环
            do {
                temp_spc = temp_spc->next;
            } while (temp_spc != NULL && temp_spc->Ring_Active_Amount == 0);
            
           //分析查询结果
            //结果1：如果发现存在活跃的环，但是新任务的优先级低于此环，拒绝该任务启用申请
            if (temp_spc != NULL && TOR_Priority_Get(TOR) < temp_spc->Ring_Priority) {
                return -1;
            }
            //结果2：如果SPC链不存在活跃的环，或者存在活跃的环并且新任务的优先级等于或高于此环，则接受任务请求
            //寻找一个最低优先级挂起态的TOR任务
			//两种策略，如果存在挂起的那么优先删除挂起的任务，如果没有那么删除一个就绪状态的任务
			
            TASK_ORBIT_RING* temp_tor = TOR_Lowest_Enable_Designated_Status_Find(TASK_OBSTRUCT);
            if (!temp_tor) {
				temp_tor = TOR_Lowest_Enable_Designated_Status_Find(TASK_READY);
				if(!temp_tor)
					return -1;//SPC链出现了异常
            }else{
				TASK_ORBIT_RING* temp_tor2 = TOR_Lowest_Enable_Designated_Status_Find(TASK_READY);
				if(temp_tor2 && TOR_Status_Get(&temp_tor->tactics) > TOR_Status_Get(&temp_tor2->tactics)){
					temp_tor = temp_tor2;
				}						
			}
			
			
            //终止该任务
            Task_Exit_Designated(temp_tor);
            //重新申请新任务
            goto flag;
        
        //策略3
        #elif TASK_LOAD_SOLUTION == TASK_LOAD_SOLUTION_3
            //获取SPC中优先级最低的已经被启用的环
            TASK_ORBIT_RING* temp_tor = TOR_Lowest_Enable_Find();
            if (!temp_tor) {
                //不存在启用的环
                return -1;
        }
            //检测是否返回的TOR优先级高于新任务的优先级
            if (temp_tor->self_SPC->Ring_Priority >= TOR_Priority_Get(TOR)) {
                return -1;//如果高于或者等于则拒绝新任务的申请
            }
            //终止该任务
            Task_Exit_Designated(temp_tor);
            //重新申请新任务
            goto flag;
        #endif
    }
    TOR->self_SPC->Ring_Active_Amount++;
    //初始化栈
    *(--TOR->TCB.stack_top) = (stack_t)0x01000000u;   							        // xPSR bit 24 = 1
    *(--TOR->TCB.stack_top) = (stack_t)TOR->TCB.taskFunction;    						// R15 PC 
    *(--TOR->TCB.stack_top) = (stack_t)0x00000014u; 							        // R14 LR
    *(--TOR->TCB.stack_top) = (stack_t)0x00000012u; 							        // R12
    *(--TOR->TCB.stack_top) = (stack_t)0x00000003u;							            // R3
    *(--TOR->TCB.stack_top) = (stack_t)0x00000002u;							            // R2
    *(--TOR->TCB.stack_top) = (stack_t)0x00000001u; 						        	// R1
    *(--TOR->TCB.stack_top) = (stack_t)TOR->TCB.Args; 					    			// R0
    *(--TOR->TCB.stack_top) = (stack_t)0x00000011u; 						        	// R11
    *(--TOR->TCB.stack_top) = (stack_t)0x00000010u;					            		// R10
    *(--TOR->TCB.stack_top) = (stack_t)0x00000009u; 				        			// R9
    *(--TOR->TCB.stack_top) = (stack_t)0x00000008u;				            			// R8
    *(--TOR->TCB.stack_top) = (stack_t)0x00000007u;				            			// R7
    *(--TOR->TCB.stack_top) = (stack_t)0x00000006u; 			        				// R6
    *(--TOR->TCB.stack_top) = (stack_t)0x00000005u; 			        				// R5
    *(--TOR->TCB.stack_top) = (stack_t)0x00000004u;				            			// R4
    //任务设置为就绪态
    TOR_Status_Set(&TOR->tactics, TASK_READY);
    TOR->Task_Enable_Flag = TOR_ENABLE_FLAG;//任务设置为启用状态
	TaskQueueUpdated = NEW_TASK_AVAILABLE;  //有新任务进入标志使能
    return 0;
}
//-----------------------------------------------------------任务的创建和加载 结束------------------------------------------------


//-----------------------------------------------------------任务的优先级操作 开始------------------------------------------------
//修改任务优先级
//设想情形：在发生优先级逆转的前提下，如果提高优先级会导致之前的SPC被销毁，\
目前的设想是，这个函数作为API，然后再设计一个临时优先级函数，但是如果SPC环为空也不能销毁原SPC
int Task_Priority_Modify(TASK_ORBIT_RING* TOR, unsigned char Priority) {
    if (!TOR) {
        return -1;//没有此任务
    }

    if (TOR->self_SPC->Ring_Priority == Priority) {
        return -1;//想要修改的优先级和原优先级相同，不需要修改，退出
    }

    TOR_Priority_Set(TOR, Priority);//修改优先级

    SCHEDULING_PRIORITY_CHAIN* temp = TOR->self_SPC;

    if (TOR_Move(TOR)) {
        return -1;//优先级修改失败
    }

    //递减原SPC的总数，如果SPC的TOR节点为0，则销毁这个SPC
    if (--temp->Ring_Task_Amount == 0) {
        if (SPC_Delete(temp->Ring_Priority)) {
            return -1;//删除失败
        }
    }
    return 0;
}

//-----------------------------------------------------------任务的优先级操作 开始------------------------------------------------

//-----------------------------------------------------------任务的挂起与终止 开始------------------------------------------------
/*
* 任务挂起
*/
void Task_Suspended(void) {
    //获取当前探头
    SCHEDULING_PRIORITY_CHAIN* SPC = Get_SPC_Detector();
	
	__disable_irq(); // 关闭全局中断
    //将当前TOR的状态设置为挂起
    TOR_Status_Set(&SPC->Task_Ring->tactics, TASK_OBSTRUCT);
	
    /*【将系统滴答定时器计数重置】
    * 原因如下：
    * （1）重置后能够让后续的判断有充足的时间执行，避免出现一种极度极端的情况——下方代码判断为真的瞬间，
    *但是还没有执行触发器代码的这一刻，内核切换了任务，此操作将会导致判断失效（参考下方注释情况1），从而引发系统时间片浪费
    * （2）时间片不会出现浪费，即便通过重置时间片，但是触发器执行后会立即进入任务切换状态
    */

    //滴答系统定时器重置代码...
    //Reset_Time_Slice_Timing();
	

    /*检测任务是否挂起
    *设置此判断的原因如下:
    * 当一个任务状态被设置为了挂起态后，本质上只是改变了标志位，当前任务依旧运行，所以需要切换任务彻底让系统认为此任务进入挂起态。
    * 所以此时有可能出现两种情况：
    * （1）任务设置为挂起态后，在这个临界区内核正好发出了任务切换信号，虽然任务成功挂起但是后续主动激活任务切换的代码还没有执行。
    *当下一次此任务再次激活进入活跃态的时候，顺序执行会执行到任务切换触发器会导致任务立刻切换，从而造成了任务被错误的跳过了一个时间片。
    * （2）任务设置为挂起态后，时间片还很充裕，那么接下来就会执行任务切换触发器，在预期中正常挂起任务
    * 【解决方案】
    * （1）在此处增加一个判断，因为如果任务出现了情况1，那么再次接着执行该任务时，必定处于活跃态，所以此时不应该再执行触发器代码
    * （2）当此时的判断发现状态为挂起时，在挂起状态下执行代码的异常情况只会发生在首次将任务挂起的时候，此时挂起操作还没有完成，
    *所以应当继续执行当前的主动触发任务触发器的代码
    */
    //当前SPC任务活跃数自减
    SPC->Ring_Active_Amount--;
	
	__enable_irq(); // 开启全局中断
	
    if (TOR_Status_Get(&SPC->Task_Ring->tactics) == TASK_OBSTRUCT) {
        //如果任务挂起，则立刻激活任务切换器，切换任务。
        User_Initiated_Switch_Trigger();
    }
}
//挂起指定任务
int Task_Suspended_Designated(TASK_ORBIT_RING* TOR) {
    if (!TOR) {
        return -1;//此任务未被注册
    }
    /*此函数相比悬挂自身的函数，不需要复杂的分类讨论逻辑
    * 可能遇到的异常情况
    * （1）当判断任务未启用状态时，此时的直接返回，即便刚过这个判断的临界点，
    *内核切换其他任务启用了指定任务也不影响，因为在判断的那一刻已经确定了任务状态，之后的其他状况，当前任务不需要为他们负责。
    * （2）在判断挂起时，在判断是否处于活跃态的临界点，内核经过多次任务切换回来。
    * 如果目标任务变为了挂起态，那么该函数的操作不影响。
    * 如果目标任务依旧为活跃态，那么该函数修改目标状态能正常进行。
    */
    //检测任务是否被启用
    if (TOR->Task_Enable_Flag == TOR_DISABLE_FLAG) {
        //如果任务没有被启用，则返回
        return -1;
    }
    //设置挂起
    if (TOR_Status_Get(&TOR->tactics) != TASK_OBSTRUCT) {
        TOR_Status_Set(&TOR->tactics, TASK_OBSTRUCT);
    }
    //目标环的SPC的任务活跃数减少
    TOR->self_SPC->Ring_Active_Amount--;
    return 0;
}

/*
* 任务结束
*/
//结束当前任务
int Task_Exit(void) {
    //获取当前探头
    SCHEDULING_PRIORITY_CHAIN*SPC = Get_SPC_Detector();

    //关闭任务
    if (TOR_Disable(SPC->Task_Ring)) {
        return -1;//堆栈释放失败
    }

    /*后续添加任务切换触发器，有两种情况
    * （1）极端情况，当全局中断打开的瞬间，内核发生了任务切换，此时由于该任务已经做好了关闭（资源释放）准备，
    *所以任务将在脱离CPU占有后彻底进入关闭状态。
    * （2）正常情况，当时间片充裕的时候，此时代码执行到触发器，将立刻进入任务切换，在解除了CPU占有后进入关闭状态
    */
    User_Initiated_Switch_Trigger();

    return 0;
}
//结束指定任务
int Task_Exit_Designated(TASK_ORBIT_RING* TOR) {
    if (!TOR) {
        return -1;//此任务未被注册
    }
    /*
    * （1）通过任务检测，发现任务没有启用，此时立即返回，即便在判断返回的临界点启用了目标任务，但是当前任务依旧不用对其负责
    * （2）在检测任务启用判断完成和关闭任务判断开始的临界点，如果任务由于某些原因关闭，
    * 
    */
    //关闭任务
    if (TOR_Disable(TOR)) {
        return -1;//堆栈释放失败
    }
    /*这里使用不触发器原因如下：
    * 第一，使用此函数的任务的重要性较高，任务加载函数需要使用到该函数，
    *在加载策略中，需要删除低优先级任务来加载高优先级的情况都比较紧急。如果在极限状态下，由于一次内核的任务切换被屏蔽，新任务可以更快进入运行
    * 第二，为了尽可能让高优先级任务占有资源，其他低优先级甚至同优先级任务推迟一个时间片也是有意义的。
    */
    return 0;
}
//-----------------------------------------------------------任务的挂起与终止 结束------------------------------------------------


//-----------------------------------------------------------任务唤醒 开始------------------------------------------------
//任务唤醒
int Task_Wake(TASK_ORBIT_RING* TOR) {
    if (!TOR) {
        return -1;//此任务未被注册
    }
    //检测任务是否被启用
    if (TOR->Task_Enable_Flag == TOR_DISABLE_FLAG) {
        //如果任务没有被启用，则返回
        return 0;
    }
	__disable_irq(); // 关闭全局中断
	//检查任务状态
	if(TOR_Status_Get(&TOR->tactics) == TASK_OBSTRUCT){
	//只有当任务处于挂起的时候才能唤醒
		TOR_Status_Set(&TOR->tactics, TASK_READY);
		//自增该SPC环的活跃任务数量
		TOR->self_SPC->Ring_Active_Amount++;
		//有任务唤醒，将新任务标志位使能
		TaskQueueUpdated = NEW_TASK_AVAILABLE;
	}
	__enable_irq(); // 开启全局中断
    return 0;
}
//-----------------------------------------------------------任务唤醒 结束------------------------------------------------


//-----------------------------------------------------------任务名与地址的查找 开始------------------------------------------------

/*
* 任务查找
*/

TASK_ORBIT_RING* Task_Find(char* Task_Name) {
    return HashTable_Find(Task_Name);
}
//-----------------------------------------------------------任务名与地址的查找 结束------------------------------------------------

//获取当前任务的句柄
TASK_ORBIT_RING* Task_Current_Handle(void){
	SCHEDULING_PRIORITY_CHAIN* spc = Get_SPC_Detector();
	return spc->Task_Ring;
}







//特殊优先级任务创建(此任务独立于所有优先级之外，与优先级任务交替执行)
#if SPECIAL_PRIORITY_CONFIG	== SPECIAL_PRIORITY_ENABLE
int Special_Priority_Task_Create(\
	TASK_FUNCTION taskFunction,\
	void* Args,\
	unsigned char* icon,\
	char* Task_name,\
	StackDepth size){
	//检查任务名
	char task_name_temp[TASK_NAME_SIZE + 1];
    task_name_temp[TASK_NAME_SIZE] = '\0';
    if (strlen(Task_name) <= TASK_NAME_SIZE) {
        for (unsigned char i = 0; i < strlen(Task_name) + 1; i++) {
            task_name_temp[i] = Task_name[i];
        }
    }
    else {
        return -1;//命名过长
    }
    //任务名是否存在重复
    if (HashTable_Find(task_name_temp)) {
        return -4;//任务名已经存在
    }
	#if HEAP_SCHEME == HEAP_SOLUTION_1  	
	TASK_ORBIT_RING* TOR = (TASK_ORBIT_RING*)hj_malloc(sizeof(TASK_ORBIT_RING));
	#endif
	if(!TOR){
		return -1;//空间分配失败
	}
	
	//配置TOR
	TOR->icon = icon;
	TOR->next = NULL;
	TOR->previous = NULL;
	TOR->self_handle = TOR;
	TOR->self_SPC = SPC_Head;
	TOR_Status_Set(&TOR->tactics,TASK_OBSTRUCT);
	TOR->Task_Enable_Flag = TOR_DISABLE_FLAG;
	strcpy(TOR->Task_name, task_name_temp);
	//TCB
	TOR->TCB.Args = Args;
	TOR->TCB.stack_bottom = NULL;
	TOR->TCB.stack_size = size;
	TOR->TCB.stack_top = NULL;
	TOR->TCB.taskFunction = taskFunction;
	
	//插入hash表
	if(HashTable_Insert(TOR->self_handle)){	
        //插入失败
    #if HEAP_SCHEME == HEAP_SOLUTION_1  
        hj_free(TOR, sizeof(TASK_ORBIT_RING));
    #endif

        return -1;
    }
		
	if(SPC_Head->Task_Ring == NULL){
		SPC_Head->Task_Ring = TOR;
		SPC_Head->TOR_Leader = TOR;
		TOR->next = TOR;
		TOR->previous = TOR;
		return 0;
	}
	TOR->next = SPC_Head->TOR_Leader;
	TOR->previous = SPC_Head->TOR_Leader->previous;
	TOR->next->previous = TOR;
	TOR->previous->next = TOR;
	return 0;
}
#endif



//互斥锁定时结构体
/*
* （1）只有阻塞式的锁才会进入互斥锁等待队列
* （2）任务切换时，会将所有定时互斥锁的时间自减
* （3）定时结构体按照优先级排列。
*/


/*************************
* 进程同步
*************************/
//-------------------------------------------------------进程同步 开始--------------------------------

//在任务等待链表中添加一个节点
Task_Wait_Tracker* TWT_Add(TASK_ORBIT_RING* TOR, unsigned int systick_time, void* Synchronization_Primitives, unsigned char block_type) {
	#if HEAP_SCHEME == HEAP_SOLUTION_1  //堆区方案1
	Task_Wait_Tracker* node = (Task_Wait_Tracker*)Ink_malloc(sizeof(Task_Wait_Tracker));//申请空间
	#endif
	if (!node) {
		//空间申请失败
		return NULL;
	}
	//初始配置
	node->TOR = TOR;
	node->systick_time = systick_time;
	//node->mutex = mutex;
	node->Synchronization_Primitives = Synchronization_Primitives;
	node->block_type = block_type;
	node->enable_flag = 1;
	node->next = NULL;

	//入列
	if (!TWT_Header) {
		//如果TWT没有节点
		TWT_Header = node;
	}
	else {
		Task_Wait_Tracker* fast = TWT_Header;
		Task_Wait_Tracker* slow = NULL;
		while (fast != NULL) {
			if (TOR->self_SPC->Ring_Priority > fast->TOR->self_SPC->Ring_Priority) {
				break;
			}
			slow = fast;
			fast = fast->next;
		}
		node->next = fast;
		slow->next = node;
	}
	return node;
}

//删除指定任务等待节点
void TWT_Delete(Task_Wait_Tracker* TWT_Anchor_Point) {
	if (!TWT_Header) {
		//TWT链表为空
		return;
	}

	Task_Wait_Tracker* fast = TWT_Header;
	Task_Wait_Tracker* slow = NULL;

	if (TWT_Anchor_Point == TWT_Header) {
		//如果要删除的节点是头结点
		TWT_Header = TWT_Header->next;
	}
	else {
		//如果删除的节点不是头结点
		while (fast != NULL) {
			if (fast == TWT_Anchor_Point) {
				break;
			}
			slow = fast;
			fast = fast->next;
		}

		slow->next = fast->next;//剔除目标TWT
	}
#if HEAP_SCHEME == HEAP_SOLUTION_1  //堆区方案1
	Ink_free(fast, sizeof(Task_Wait_Tracker));//释放堆区空间
#endif
	return;
}



//初始化普通互斥锁（注意，该函数只能在main中使用）
Mutex_t* Mutex_Init(void) {
	#if HEAP_SCHEME == HEAP_SOLUTION_1 
	Mutex_t* Mutex = (Mutex_t*)Ink_malloc(sizeof(Mutex_t));
	#endif
	if(!Mutex){
		return Mutex;
	}
	Mutex->Possessor = NULL;
	Mutex->Status = 0;
	Mutex->Initialization_flag = 0;
	return Mutex;
}



//普通互斥锁上锁(不阻塞)
int Mutex_Lock_Non_Blocking(Mutex_t* Mutex) {
	if (Mutex->Initialization_flag) {
		return -2;//如果互斥锁没有初始化
	}

	//获取当前SPC
	SCHEDULING_PRIORITY_CHAIN* SPC = Get_SPC_Detector();


	//如果是首次上锁
	/*【极端情况】
		* （1）如果此时的通过了当前if判断，但是还没有设置所有权的时候，任务切换，
		* 另外一个任务存在抢先设置所有权的情况，那么当任务切换回来的时候，任务的所有权就会被当前出现覆盖的情况
	*/
	//关闭全局中断，保证原子性
	//关闭全局中断代码...
	__disable_irq();
	if (Mutex->Possessor == NULL) {
		Mutex->Possessor = SPC->Task_Ring;//更新锁的所有权
		Mutex->Status = 1;//设置为上锁状态
		
		//开启全局中断代码...
		__enable_irq();
		return 0;
	}

	//判断是否上锁
	if (Mutex->Status == 0) {
		//如果没有上锁
		Mutex->Possessor = SPC->Task_Ring;//更新锁的所有权
		Mutex->Status = 1;//设置为上锁状态
		//上锁成功
		//开启全局中断代码...
		__enable_irq();
		return 0;
	}
	//开启全局中断代码...
	__enable_irq();
	//如果已经上锁返回-1
	return -1;
	
}


//普通互斥锁上锁(阻塞),以时间片为单位（1时间片长度、2时间片长度），如果为0，则永久阻塞，直到互斥锁资源释放
int Mutex_Lock(Mutex_t* Mutex,unsigned int Block_Time) {
	//检测互斥锁是否初始化
	if (Mutex->Initialization_flag) {
		return -2;//如果互斥锁没有初始化
	}

	//获取当前SPC
	SCHEDULING_PRIORITY_CHAIN* SPC = Get_SPC_Detector();

	//如果被同一个TOR上锁
	if (Mutex->Status == 1 && Mutex->Possessor == SPC->Task_Ring) {
		/*
		* 由于互斥锁已经被当前任务上锁，直接返回。
		*这里不需要关闭全局中断，因为如果被当前任务上锁，
		*那么其他任务是无法解锁的，所以不会出现线程不安全
		*/
		return 0; 
	}


	/*接下来争夺互斥锁的过程可能引发线程不安全，所以代码需要处于临界区，也就是关闭全局中断
	* 
	* 【极端情况】
	* （1）如果此时的通过了当前if判断，但是还没有设置所有权的时候，任务切换，
	* 另外一个任务存在抢先设置所有权的情况，那么当任务切换回来的时候，任务的所有权就会被当前出现覆盖的情况
	*/

	//【大类情况1：首次争夺互斥锁】
	__disable_irq();
	//判断是否首次上锁
	if (Mutex->Possessor == NULL) {
		/*只有当所有权为NULL的时候是首次上锁.
		* 因为当互斥锁是其他任务释放的时候，只是上锁状态更改，所有权的值没有改变，而是等待新值覆盖
		*/
		Mutex->Possessor = SPC->Task_Ring;//更新锁的所有权
		Mutex->Status = 1;//设置为上锁状态
		//恢复全局中断
		__enable_irq();
		return 0;
	}
	//开启全局中断，首次上锁判断后非首次判断是两大类情况，中间允许发生抢夺
	__enable_irq();
 
	/*【大类情况2：非首次争夺互斥锁】
	*非首次争夺互斥锁，在永久阻塞和计时内的定时阻塞会一直在争夺互斥锁过程循环
	*/

	//声明争夺互斥锁的必要变量
	unsigned char Task_Wait_Flag = 0;//是否加入了等待队列标志, 0为没有加入，非0为已经加入
	Task_Wait_Tracker* TWT_Anchor_Point = NULL;//这个指针用于定位当前任务在等待队列中的位置

FLAG:;//争抢互斥锁的跳转标签
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;//关闭任务调度（防止调度命令被挂起，开启中断的临界点进入任务调度，引发和任务挂起的死锁）
	//关闭全局中断，原子操作不允许被打断
	__disable_irq();
	//如果互斥锁没有被上锁
	if (Mutex->Status == 0) {
		//立即抢夺互斥锁的所有权
		Mutex->Possessor = SPC->Task_Ring;//更新锁的所有权
		Mutex->Status = 1;//设置为上锁状态
		if (Task_Wait_Flag) {
			//如果当前任务添加过TWT,从TWT链表移除节点,成功抢夺互斥锁后，不管是定时还是永久互斥锁，都不需要再使用TWT节点，所有应该删除
			TWT_Delete(TWT_Anchor_Point);
		}
		SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;//开启任务调度
		//开启全局中断
		__enable_irq();
		return 0;
	}

	//如果互斥锁已经上锁，那么则进入阻塞状态，并将信息再入任务等待链表中
	//根据定时阻塞与永久阻塞分类
	if (!Task_Wait_Flag) {//判断是否已经加入了等待队列，如果没有加入则申请加入。
		if (Block_Time == 0) {
			//永久阻塞
			
			TWT_Anchor_Point = TWT_Add(SPC->Task_Ring, 0, Mutex, 0);
		}
		else {
			//定时阻塞
			TWT_Anchor_Point = TWT_Add(SPC->Task_Ring, Block_Time, Mutex, 1);
		}

		//检查是否加入TWT链成功
		if (!TWT_Anchor_Point) {
			//开启全局
			__enable_irq();
			return -1;//队列加入失败
		}

		Task_Wait_Flag = 1;//更新等待队列入队标志
	}

	//重置系统滴答计时器，预防在下方代码开启中断的瞬间，函数在被挂起之间先被唤醒，错过唤醒条件，从而引发死锁
	SysTick->VAL = 0;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	//开启全局
	__enable_irq();
	//挂起当前任务
	Task_Suspended();

	//判断被唤醒的方式
	//方式1：定时时间到
	if (TWT_Anchor_Point->block_type == 1 && TWT_Anchor_Point->systick_time == 0) {
		//判断阻塞的类型为定时阻塞          //判断定时时间为0

		//从队列中移除TWT
		TWT_Delete(TWT_Anchor_Point);
		return 0;
	}
	//方式2：互斥锁释放，被上一任所有者唤醒

	//当任务被唤醒的时候，立刻重新争夺互斥锁控制权
	goto FLAG;
}

 
//普通互斥锁解锁
int Mutex_Unlock(Mutex_t* Mutex) {

	/*【大类情况1】
	* 验证当前互斥锁是否进行过初始化
	*/
	if (Mutex->Initialization_flag) {
		return -2;//未初始化
	}

	//获取当前SPC
	SCHEDULING_PRIORITY_CHAIN* SPC = Get_SPC_Detector();

	/*【大类情况2】
	* 如果目标互斥锁处于未解锁状态，则立刻返回
	*/

	//关闭全局中断
	__disable_irq();
	//验证是否上锁，（1）检查锁的状态，（2）检查所有权是否为NULL
	if (!Mutex->Status || Mutex->Possessor == NULL) {
		//没有上锁或者初始化之后还没有任务加锁过
		//开启全局中断
		__enable_irq();
		return 0;
	}


	/*【大类情况3】
	* 如果当前互斥锁已经被加锁，则考虑验证解锁对象是否是锁的所有者
	*/

	//验证锁的所有者
	if (Mutex->Possessor != SPC->Task_Ring) {
		//所有权验证失败
		//开启全局中断
		__enable_irq();
		return -1;
	}

	//互斥锁的所有者申请解锁
	//解除上锁状态
	Mutex->Status = 0;
	//唤醒一个等待的节点
	
	Task_Wait_Tracker* TWT_Anchor_Point = TWT_Header;

	while (TWT_Anchor_Point != NULL) {
		//验证如果找到一个节点处于使能状态，并且其保存的互斥锁地址等于当前的互斥锁地址（即同一个互斥锁），则唤醒该任务
		if (TWT_Anchor_Point->enable_flag == 1 && TWT_Anchor_Point->Synchronization_Primitives == Mutex) {
			//唤醒该任务
			Task_Wake(TWT_Anchor_Point->TOR);
			////失能这个节点的状态
			//TWT_Anchor_Point->enable_flag = 0;
			//开启全局中断
			__enable_irq();
			return 0;
		}
		//更新锚点
		TWT_Anchor_Point = TWT_Anchor_Point->next;
	}

	//开启全局中断
	__enable_irq();
	return 0;
}




/*【该函数实现的功能如下】
* （1）该函数应当在任务切换的临界区使用
* （2）该函数针对的是阻塞类互斥锁，即永久阻塞和定时阻塞
* （3）每一次调用该函数，应当将定时阻塞类的节点systick_time自减
* （4）为了提高效率，应当先自减再判断，如果定时阻塞的systick_time的为0时，应当唤醒对应的任务，（目前的想法是唤醒的同时也删除该节点）
* （5）当链表节点的定时到了之后，立刻将节点数据结构中的enbale_flag标志设置为失能【目前的想法是让节点在上锁函数中删除】
*/

//更新同步原语【目前没有发现bug】
void Synchronization_Primitives_Update(void) {
	//检查任务等待节点是否为空
	if (!TWT_Header) {
		//检测为空，则立刻返回
		return;
	}
	//定义一个锚点，用于对TWT链进行遍历
	Task_Wait_Tracker* TWT_Anchor_Point = TWT_Header->next;

	//遍历条件：当前没有遍历到TWT链的末尾
	while (TWT_Anchor_Point != NULL) {

		//判断当前锚点类型是定时阻塞还是永久阻塞,同时判断当前的锚点是否被启用
		if (TWT_Anchor_Point->block_type == 0 || TWT_Anchor_Point->enable_flag == 0) {
			//如果为永久阻塞或者没有被启用,更新锚点并重新进入循环遍历中
			TWT_Anchor_Point = TWT_Anchor_Point->next;
			continue;
		}

		//如果是定时类阻塞
		//先自减一个时间单位，因为所有的时间必须是大于0的，而永久阻塞在上方代码过滤，所以不会出现异常情况
		if (--TWT_Anchor_Point->systick_time == 0) {
			//当时间为0的时候，则需要立刻唤醒指定的任务
			//将当前节点设置为失能，防止在没有被删除的条件下被错误调用（注意，节点无需由当前函数释放内存，而是其他函数会在接收到失能信息后删除）
			TWT_Anchor_Point->enable_flag = 0;
			//唤醒任务
			Task_Wake(TWT_Anchor_Point->TOR);
		}

		//如果时间不为0，那么重新进入循环
		TWT_Anchor_Point = TWT_Anchor_Point->next;
	}
	return;
}

//--------------------------------------------------------------------------互斥锁结束------------------------------------

//初始化信号量
Semaphore_t* Semaphore_Init(int MaxCount,int InitCount) {
	#if HEAP_SCHEME == HEAP_SOLUTION_1 
	Semaphore_t* temp = (Semaphore_t*)Ink_malloc(sizeof(Semaphore_t));
	#endif
	if(!temp){
		return temp;
	}
	temp->Max_Count = MaxCount;
	temp->Current_Count = InitCount;
	temp->Initialization_flag = 0;
	return temp;
}
//以不阻塞的方式申请一个信号量
int  Semaphore_Applay_Non_Blocking(Semaphore_t* Semaphore) {
	if (Semaphore->Initialization_flag) {
		return -2;//如果互斥锁没有初始化
	}

	//获取当前SPC
	SCHEDULING_PRIORITY_CHAIN* SPC = Get_SPC_Detector();

	//如果是首次上锁
	/*【极端情况】
		* （1）如果此时的通过了当前if判断，但是还没有设置所有权的时候，任务切换，
		* 另外一个任务存在抢先设置所有权的情况，那么当任务切换回来的时候，任务的所有权就会被当前出现覆盖的情况
	*/
	//关闭全局中断，保证原子性
	//关闭全局中断
	__disable_irq();
	//如果资源有剩余的情况
	if (Semaphore->Current_Count > 0) {
		Semaphore->Current_Count--;
		//开启全局中断
		__enable_irq();
		return 0;
	}
	//开启全局中断
	__enable_irq();
	//申请失败，立刻退出

	//如果已经上锁返回-1
	return -1;

}

//计数量申请（阻塞版本）
int Semaphore_Applay(Semaphore_t* Semaphore, int Block_Time) {
	if (Semaphore->Initialization_flag) {
		return -2;//信号量没有初始化
	}

	//获取当前SPC
	SCHEDULING_PRIORITY_CHAIN* SPC = Get_SPC_Detector();
	//声明争夺互斥锁的必要变量
	unsigned char Task_Wait_Flag = 0;//是否加入了等待队列标志, 0为没有加入，非0为已经加入
	Task_Wait_Tracker* TWT_Anchor_Point = NULL;//这个指针用于定位当前任务在等待队列中的位置

FLAG:;//争抢信号量的跳转标签
	/*
	* 【大类情况1：计数量大于0，还可被枷锁】
	*/
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;//关闭任务调度（防止调度命令被挂起，开启中断的临界点进入任务调度，引发和任务挂起的死锁）
	//关闭全局中断
	__disable_irq();
	if (Semaphore->Current_Count > 0) {
		Semaphore->Current_Count--;
		if (Task_Wait_Flag) {
			//如果加入过TWT，那么从TWT中移除节点
			TWT_Delete(TWT_Anchor_Point);
		}
		//开启全局
		__enable_irq();
		return 0;
	}
	
	
	/*
	* 【大类情况1：计数量大于0，所有资源已经被申请，任务将会陷入等待】
	*/
	//判断是否加入了等待队列
	if (!Task_Wait_Flag) {
		//如果没有加入
		if (Block_Time == 0) {
			//永久阻塞
			TWT_Anchor_Point = TWT_Add(SPC->Task_Ring, 0, Semaphore, 0);
		}
		else {
			//定时阻塞
			TWT_Anchor_Point = TWT_Add(SPC->Task_Ring, Block_Time, Semaphore, 1);
		}
			
		//检查是否加入TWT链成功
		if (!TWT_Anchor_Point) {
			//开启全局
			__enable_irq();
			return -1;//队列加入失败
		}
			
		Task_Wait_Flag = 1;//更新等待队列入队标志
	}
	//如果已经加入
	//重置系统滴答计时器，预防在下方代码开启中断的瞬间，函数在被挂起之间先被唤醒，错过唤醒条件，从而引发死锁
	SysTick->VAL = 0;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	//开启全局
	__enable_irq();
	//挂起当前任务
	Task_Suspended();

	//判断被唤醒的方式
	//方式1：定时时间到
	if (TWT_Anchor_Point->block_type == 1 && TWT_Anchor_Point->systick_time == 0) {
		//判断阻塞的类型为定时阻塞          //判断定时时间为0

		//从队列中移除TWT
		TWT_Delete(TWT_Anchor_Point);
		return 0;
	}
	//方式2：信号量资源恢复唤醒

	//当任务被唤醒的时候，立刻重新争夺互斥锁控制权
	goto FLAG;

}

//计数量归还
int Semaphore_Give(Semaphore_t* Semaphore) {
	//检查是否初始化
	if (Semaphore->Initialization_flag) {
		//如果没有初始化，则返回错误
		return -2;
	}

	//检查可用资源是否满了
	if (Semaphore->Current_Count == Semaphore->Max_Count) {
		//如果可用资源是满的，那么不必再释放
		return 0;
	}

	/*当有任务占用可用资源时
	* 【情况1：部分可用资源被占用，但是还有部分资源剩余】
	* 【情况2：所有的可用资源都被占用】
	*/

	//关闭全局中断
	__disable_irq();

	//情况1
	if (Semaphore->Current_Count > 0) {
		//资源有剩余，不需要考虑等于最大资源的情况，因为已经被过滤了
		//由于资源有剩余，所以也不存在任务处于等待阻塞的情况，所以只需要释放信号量即可
		Semaphore->Current_Count++;
		//开启全局中断
		__enable_irq();
		return 0;
	}

	//情况2，需要将等待队列中的一个任务唤醒 

	Task_Wait_Tracker* TWT_Anchor_Point = TWT_Header;

	while (TWT_Anchor_Point != NULL) {
		//验证如果找到一个节点处于使能状态，并且其保存的互斥锁地址等于当前的互斥锁地址（即同一个互斥锁），则唤醒该任务
		if (TWT_Anchor_Point->enable_flag == 1 && TWT_Anchor_Point->Synchronization_Primitives == Semaphore) {
			//唤醒该任务
			Task_Wake(TWT_Anchor_Point->TOR);
			////失能这个节点的状态
			//TWT_Anchor_Point->enable_flag = 0;
			Semaphore->Current_Count++;
			//开启全局中断
			__enable_irq();
			return 0;
		}
		//更新锚点
		TWT_Anchor_Point = TWT_Anchor_Point->next;
	}
	Semaphore->Current_Count++;
	//开启全局中断
	__enable_irq();
	return 0;
}


//----------------------------------------------进程同步 结束-----------------------------------------
