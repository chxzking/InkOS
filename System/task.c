#include "task.h"
//���������ڱ��ļ��е�����
//����
SCHEDULING_PRIORITY_CHAIN* SPC_Head = NULL;                                     //SPCͷ�ڵ�
SCHEDULING_PRIORITY_CHAIN* SPC_Detector = NULL;                                 //SPC̽ͷ
INSPECTOR SPC_TOR_Inspector[2];                                                 //˫Ѳ��Ա(��һλΪ��ȫѲ�飬�ڶ�λ��ԾѲ��)
HashTable Hash_TOR;                                                             //����һ�����TOR���ƺ�λ�ù�����hash��
unsigned char TaskQueueUpdated = NEW_TASK_NOT_AVAILABLE;                        //�Ƿ������������ı�־λ���¼��������������
Task_Wait_Tracker* TWT_Header = NULL;											//�ȴ�����ͷָ��
Stack_Overflow_t STACK_OVERFLOW_FLAG = STACK_SAFETY;											//ջ����ź�

//��������
unsigned int Task_Idle_Stack[TASK_IDLE_STACK_SIZE] = { 0 };
TASK_ORBIT_RING TOR_Idle;
SCHEDULING_PRIORITY_CHAIN SPC_Idle;

//����
int SPC_Add(unsigned char Priority);                                            //���
int SPC_Delete(unsigned char Priority);                                         //ɾ��
int TOR_Create(TASK_FUNCTION taskFunction, \
    void* Args, \
    unsigned char Priority, \
    unsigned char* icon, \
    char* Task_name, \
    StackDepth size, \
    TASK_ORBIT_RING** node);                                                    //���һ��TOR�ڵ�(!!!!!!!!!!!ע�⣬ֻ�������һ��TOR����û���ڹ�ϣ������ӽڵ㣬Ŀǰ�滮��δ����Task_Create�н������߽��)

int TOR_Move(TASK_ORBIT_RING* TOR);                                             //TOR�ƶ������ȼ����ָı�󣬴ӵ�ǰSPC�ƶ���ָ�����ȼ���SPC��

/*************************
* Ѳ��Ա����
**************************/
///////////////////////////////////////////////////////////��ʼ//////////////////////////////////////////////////////////    
//��ʼ��Ѳ��
void Inspector_Init(void) {
    //��ȫѲ�飨���������е�����
    //��ԾѲ�飨�����Ǳ����á����ؽ����ڴ�����񡿣�
    /*
    * Ѳ��Ա��ʼ��Ϊ����NULL���뷨���£�
    *��1�� ����������£�Ѳ��Ա��SPC���ھ����˵�һ��ִ�к�Ͳ������ٳ��ֵ���NULL�������
    ��Ϊ�ڲ�ѯ�ĺ����У�������NULL�ͻᱻ���̸���Ϊ��������Ҫ���SPC��ַ��
    ������ؽ��������SPC=NULL��ô�ʹ�����SPC�����ֵ������⣬���·���NULL����{�����ж�SPC���Ƿ�����}
    * ��2��SPC�쳣�����������
    1��SPCû�нڵ㣬��ôѲ���״��ƶ�����NULL����ʱ���̷���Ѳ��Ա��
    2��SPC������������������ЧSPC�ڵ�
    */
    for (unsigned char i = 0; i < 2; i++) {
        SPC_TOR_Inspector[i].SPC = NULL;
        SPC_TOR_Inspector[i].TOR = NULL;
    }
}
//������ȫѲ��(Ѳ��Ա���̸���Ϊһ���ڵ�,�����ظ�ֵ)
INSPECTOR Complete_Inspector_Forword(void) {
    //�״�ִ��
    if (SPC_TOR_Inspector[0].SPC == NULL) {
        SPC_TOR_Inspector[0].SPC = SPC_Head->next;//��Ѳ��Ա�ƶ����׸�SPC�ڵ㡣
        //��ôѰ�ҵ�һ����Ч����������TOR�Ľڵ㣬ע����SPC������Ч�ڵ�ԶԶ������Ч�ڵ㣩��SPC�ڵ�
        while (SPC_TOR_Inspector[0].SPC == NULL || SPC_TOR_Inspector[0].SPC->Ring_Task_Amount == 0) {
            /*��������е�NULL���ж��������������
            * ��1��SPC��Ϊ�գ���ô�ͻ����SPC_Head->next=NULL�����
            * ��2��SPC�����������������е�SPC������ЧSPC�ڵ㣬����Ѳ��Ա��β�������쳣��
            */
            if (SPC_TOR_Inspector[0].SPC == NULL) {
                //���̷��ذ��������Ѳ����
                return SPC_TOR_Inspector[0];
            }
            SPC_TOR_Inspector[0].SPC = SPC_TOR_Inspector[0].SPC->next;
        }
        //Ĭ�϶�ȡSPC��TOR�Ļ���
        SPC_TOR_Inspector[0].TOR = SPC_TOR_Inspector[0].SPC->TOR_Leader;
        return SPC_TOR_Inspector[0];
    }


    //���״�ִ��
    /*
    * ���״�ִ�к���ζ�ţ�SPC������������²����ٳ��ֿ����������ЧSPC����������Բ���Ҫ�ٿ���Ѳ��Ա���ִ��������
    * Ѳ��Ա����ͣ����SPC�ϣ�������ҪĿ����TOR������֮��ĺ�����TOR�������Լ���SPC��ͬSPC����
    */

    //���1��ͬSPC������,�ڻ��ڵ����1��TOR�У������ڻ�β��Ѳ���ƶ���ͬ������һ��λ��
    if (SPC_TOR_Inspector[0].SPC->Ring_Task_Amount > 1 &&//�жϻ��ڵ�������1
        SPC_TOR_Inspector[0].TOR->next != SPC_TOR_Inspector[0].SPC->TOR_Leader) {//�жϻ��ڵ㲻�ǻ�β
        SPC_TOR_Inspector[0].TOR = SPC_TOR_Inspector[0].TOR->next;//Ѳ���ƶ�����һ��λ��
    }

    //���2����SPC��������ʱ�Ѿ�����TOR�����ٽ�㣩��������Ľڵ�������1�Ҵ��ڻ�β  ���߻��ڵ�����Ϊ1��������spc��ָ����SPC�еĻ���
    else if ((SPC_TOR_Inspector[0].SPC->Ring_Task_Amount > 1 &&//�ٽ��1��TOR���Ľڵ�������1�����ǵ�ǰ�Ѿ���������SPC�Ļ�β�����������µ�SPC
        SPC_TOR_Inspector[0].TOR->next == SPC_TOR_Inspector[0].SPC->TOR_Leader) ||
        //�ٽ��2��TOR��������ֻ��1������ô����TOR���׵�ͬʱҲ���ڻ�β�����Լ�������SPC
        SPC_TOR_Inspector[0].SPC->Ring_Task_Amount == 1) {
        /*Ѱ�ҵ����ʵ�SPC�ڵ㣬���ƶ���һ��SPC���ж����Ƿ���Ч����Ч���أ���Ч�����ѭ�����ң�ֱ�����ַ���Ҫ�����SPC
        *������֮ǰ�Ѿ���������Ч��SPC�����Բ�����α���һ�����ٴγ�����Ч��SPC�������ܳ�����Ч�ڵ㣩
        */
        do {
            //�ж���һ���ڵ��Ƿ���SPCβ��NULL��������������̻ص�SPC��һ���ڵ㣬����������ѭ��
            if (SPC_TOR_Inspector[0].SPC->next == NULL) {
                SPC_TOR_Inspector[0].SPC = SPC_Head->next;
                continue;
            }
            SPC_TOR_Inspector[0].SPC = SPC_TOR_Inspector[0].SPC->next;
        } while (SPC_TOR_Inspector[0].SPC == NULL || SPC_TOR_Inspector[0].SPC->Ring_Task_Amount == 0);
        //ˢ�µ���SPC�Ļ���
        SPC_TOR_Inspector[0].TOR = SPC_TOR_Inspector[0].SPC->TOR_Leader;
    }
    return SPC_TOR_Inspector[0];
}
//������ȫѲ��(Ѳ��Ա���̻��˵���һ���ڵ�,���ظ�ֵ)
INSPECTOR Complete_Inspector_Reverse(void) {
    //�״�ִ��
    if (SPC_TOR_Inspector[0].SPC == NULL) {
        SPC_TOR_Inspector[0].SPC = SPC_Head->previous;//��Ѳ��Ա�ƶ������һ��SPC�ڵ㡣
        //Ѱ�ҵ�һ��������Ч����������TOR�Ľڵ㣬ע����SPC������Ч�ڵ�ԶԶ������Ч�ڵ㣩��SPC�ڵ�
        while (SPC_TOR_Inspector[0].SPC == NULL || SPC_TOR_Inspector[0].SPC->Ring_Task_Amount == 0 || SPC_TOR_Inspector[0].SPC == SPC_Head) {
            //��SPC��Ϊ�ջ���SPC���Ĳ�������Ч�ڵ��ʱ�����̷���
            if (SPC_TOR_Inspector[0].SPC == NULL || SPC_TOR_Inspector[0].SPC == SPC_Head) {
                SPC_TOR_Inspector[0].SPC = NULL;//ͳһ����ΪNULL�����ڱ�ʾѲ��Ա��������
                return SPC_TOR_Inspector[0];//SPC����������
            }
            SPC_TOR_Inspector[0].SPC = SPC_TOR_Inspector[0].SPC->previous;
        }
        //������ЧSPC�ڵ�󣬽�TOR����Ϊ��β���������������TOR��ֻ����һ���ڵ㣬��������ȷƥ�䵽���ʵ�TOR�ڵ�
        SPC_TOR_Inspector[0].TOR = SPC_TOR_Inspector[0].SPC->TOR_Leader->previous;
        return SPC_TOR_Inspector[0];
    }

    //���״�ִ��
    //���1��ͬSPC������,�ڻ��ڵ����1��TOR�У������ڻ��ף�Ѳ���ƶ���ͬ������һ��λ��
    if (SPC_TOR_Inspector[0].SPC->Ring_Task_Amount > 1 &&//�жϻ��ڵ�������1
        SPC_TOR_Inspector[0].TOR->previous != SPC_TOR_Inspector[0].SPC->TOR_Leader->previous) {//�жϻ��ڵ㲻�ǻ���
        SPC_TOR_Inspector[0].TOR = SPC_TOR_Inspector[0].TOR->previous;//Ѳ���ƶ�����һ��λ��
    }

    //���2����SPC��������ʱ�Ѿ�����TOR�����ٽ�㣩��������Ľڵ�������1�Ҵ��ڻ���  ���߻��ڵ�����Ϊ1��������spc��ָ����SPC�еĻ�β
    else if ((SPC_TOR_Inspector[0].SPC->Ring_Task_Amount > 1 &&//�ٽ��1����TOR�ڵ����1������²��Ҵ��ڻ���
        SPC_TOR_Inspector[0].TOR->previous == SPC_TOR_Inspector[0].SPC->TOR_Leader->previous) ||
        //�ٽ��2��TOR������Ϊ1����ʱ��TOR���ǻ������ǻ�β
        SPC_TOR_Inspector[0].SPC->Ring_Task_Amount == 1) {
        //Ѱ�ҵ����ʵ�SPC�ڵ�
        do {
            //��SPC�ڵ������SPCͷ����ʱ��Ҳ��������������ɣ������ص������β��
            if (SPC_TOR_Inspector[0].SPC->previous == SPC_Head) {
                SPC_TOR_Inspector[0].SPC = SPC_Head->previous;
                continue;
            }
            SPC_TOR_Inspector[0].SPC = SPC_TOR_Inspector[0].SPC->previous;
        } while (SPC_TOR_Inspector[0].SPC == SPC_Head || SPC_TOR_Inspector[0].SPC->Ring_Task_Amount == 0);
        SPC_TOR_Inspector[0].TOR = SPC_TOR_Inspector[0].SPC->TOR_Leader->previous;//ˢ�µ���SPC�Ļ�β
    }
    return SPC_TOR_Inspector[0];
}
//��ʽ��ȫѲ��
INSPECTOR Complete_Inspector_Static(void) {
    //Ѳ��Ա�����������ƶ�������ԭ����״̬
    return SPC_TOR_Inspector[0];
}

//�����ԾѲ��(Ѳ��Ա���̸���Ϊһ���ڵ�,�����ظ�ֵ)
INSPECTOR Active_Inspector_Forword(void){
	//�״�ִ��
	/*�ڳ�ʼ״̬��Ѳ��Ա������ΪNULL
	*/
	if(SPC_TOR_Inspector[1].SPC == NULL){
		//���SPC��û������SPC�ڵ���ֱ�ӷ��ش��ڳ�ʼ��״̬��Ѳ��Ա��
		//��ֵ������Ϊ�쳣������ж�
		if(SPC_Head->next == NULL){
			return SPC_TOR_Inspector[1];
		}
		
		//SPC����SPC�ڵ㣬��ô�ӵ�һ���ڵ㿪ʼѰ�Һ��ʵ�SPC�ڵ�
		SPC_TOR_Inspector[1].SPC = SPC_Head->next;
		while(SPC_TOR_Inspector[1].SPC != NULL && SPC_TOR_Inspector[1].SPC->Ring_Active_Amount == 0){
			SPC_TOR_Inspector[1].SPC  = SPC_TOR_Inspector[1].SPC ->next;
		}
		
		//�������
		//���1����������SPC��û�з����κξ��л�ԾTOR��SPC�����ؿ��Ա�ʶ��Ϊ�쳣�Ľ��
		if(SPC_TOR_Inspector[1].SPC == NULL){
			return SPC_TOR_Inspector[1];
		}
		
		//���2���ҵ��˾�������1����ԾTOR��SPC
		SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].SPC->TOR_Leader;
		/*�˴�����TOR���ڼ������
		*��1����ִ������TOR�������Ĺ����У�û�����񱻽��û��߱�����
		*��2��������������Ҫ���TOR�ڵ�ʱ�����������������������ڴ�˲�������л���ǡ�ý��û��߹�����������
		*/
		do{
			//������ڻ�Ծ̬�򷵻ؽ��
			SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].TOR->next;
		}while(SPC_TOR_Inspector[1].TOR != SPC_TOR_Inspector[1].SPC->TOR_Leader &&\
				SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_DISABLE_FLAG && \
		TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics) == TASK_OBSTRUCT);
		
		//����TOR���ҽ��
		/*���1���ҵ�һ������Ҫ���tor
		*���2����û���ҵ�tor֮ǰ���tor�˳��˻�Ծ̬���ٴα�����tor���׺��˳�
		*���3���ҵ���tor���������ҵ�֮��û�з��ؾ��˳��˻�Ծ̬
		*/
		if(SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_ENABLE_FLAG && TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics) != TASK_OBSTRUCT){
			//�������1
			return SPC_TOR_Inspector[1];
		}else{
			//���2��3�ϲ�Ϊһ�����������Ѳ��Ա��ֵΪ��ʶ����쳣��NULL
			SPC_TOR_Inspector[1].SPC = NULL;
			SPC_TOR_Inspector[1].TOR = NULL;
			return SPC_TOR_Inspector[1];
		}
		
	
	}
	//���״�ִ��
	
	/*���������ۡ�
	*���1����ͬһ��TOR���о���2�������ϵĽڵ�
	*��1����ǰTOR�ڵ��ڻ���
	*��2����ǰTOR�ڵ��ڻ��У�������Ծ�ڵ�������һ���ڵ�ǰTOR�ڵ��β�ڵ�֮�䡣
	*��3����ǰTOR�ڵ��ڻ��У����ǵ�ǰTOR�ڵ��β�ڵ��в����ڻ�ԾTOR�ڵ�
	*��4����ǰTOR�ڵ���ǻ�β��
	*���2��
	*��1����ǰTOR�����ҽ��е�ǰѡ�е�һ���ڵ㣬������жϹ���ǰ��һ���ڵ㱻���ѣ���ǰ��Ѳ��Ա����Ҫ����
	*/
	
	//���������1��
	if(SPC_TOR_Inspector[1].SPC->Ring_Active_Amount > 1 ){	
		//��һ����ʱ�����洢��ǰѲ��Ա��TORֵ
		TASK_ORBIT_RING* temp_TOR = SPC_TOR_Inspector[1].TOR;
		//��Ѳ��Ա��������TOR������
		SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].SPC->TOR_Leader;
		//����һ��Ѳ��Ա������һ��TOR�ڵ�ı�־
		unsigned char pass_flag = 0;
		
		do{	
			/*������־�����ζ��Ѳ��Ա�Ѿ�������֮ǰ�Ļ��ͻ�β�м��λ��
			*���ѡ�е�TOR��������״̬�����ڻ�Ծ̬����Է��ص�ǰ�Ľ��
			*������ж�ͨ�������ص��ٽ�㣬��TOR�ڵ㱻������߽��ã�Ѳ��Ա����Ҫ�Դ˸���
			*/
			if(SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_ENABLE_FLAG && pass_flag && TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics)!= TASK_OBSTRUCT){
				return SPC_TOR_Inspector[1];
			}
			/*�˴���������ǰ��TOR�ڵ㣬����pass_flag��־������λ�ò��ܸ��ģ�
			*��������������ǣ������ظ����ص�ǰTOR�ڵ㣬
			*����������˵�ǰTORʱ������pass_flag������0״̬�����Իᱻ����
			*/
			if(SPC_TOR_Inspector[1].TOR == temp_TOR){
				pass_flag = 1;
			}
			SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].TOR->next;
		}while(SPC_TOR_Inspector[1].TOR != SPC_TOR_Inspector[1].SPC->TOR_Leader);
		
		/*ִ�е��˴���ζ�ŵ�ǰ��TOR����������
		*��1����ǰ��TOR�ͻ�β֮�䲻���ڻ�Ծ��TOR��
		*��2����ǰ��TOR���ڻ�β
		*�����������
		*Ѱ���µ�SPC�ڵ㣬������Ҫ��ת�����������2��
		*/
		goto situation1_flag;
		
		
	}else{
	//���������2��
		situation1_flag:;//�˱�־��if��ת����
		situation2_flag:;//�˱�־���²������ϲ�����ת
		
		//�ӵ�ǰSPC��β���������
		//���ﲻ�õ�����ѭ�������⣬��Ϊ���������������Ϊ��SPC���бض�������һ��TOR�ǻ�Ծ��
		do{
			if(SPC_TOR_Inspector[1].SPC->next == NULL){
				//�����������SPC��ĩβ�������ص�һ���ڵ�
				SPC_TOR_Inspector[1].SPC = SPC_Head->next;
				continue;
			}
			SPC_TOR_Inspector[1].SPC = SPC_TOR_Inspector[1].SPC->next;//�ƶ�SPC�ڵ�
			//�ж��������������л�Ծ�ڵ��SPC��ֹͣ
		}while(SPC_TOR_Inspector[1].SPC->Ring_Active_Amount == 0);
		
		
		//��ǰ�����һ�������˻�ԾTOR��SPC�����Խ�Ѳ��Ա��TOR����Ϊ���ף����Ӵ˴���ʼ������
		SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].SPC->TOR_Leader;
		
		do{
			//�����ǰ��TOR�������Ҳ���������̬
			if(SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_ENABLE_FLAG && TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics) != TASK_OBSTRUCT){
				/*�˴��жϣ���������һ��TOR�ڵ㿪�������ڻ�Ծ״̬���ɷ��أ�
				*�������֮�󣬸ýڵ���������ص��ٽ�㱻������߱����ã�Ѳ��ԱҲ����Ҫ�Դ˸���
				*/
				return SPC_TOR_Inspector[1];
			}
			SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].TOR->next;
		}while(SPC_TOR_Inspector[1].TOR != SPC_TOR_Inspector[1].SPC->TOR_Leader);
		
		/*�������ִ�е��˴�����ζ���ڱ����ڼ䣬��SPC�Ļ�Ծ�ڵ��Ѿ����ٴ��ڻ�Ծ̬��
		*������Ҫ����ѡ��һ��SPC�ڵ㡣
		*/
		goto situation2_flag;
		
	}
		
}
//�����ԾѲ��(Ѳ��Ա���̻��˵���һ���ڵ�,���ظ�ֵ)
INSPECTOR Active_Inspector_Reverse(void){
	//�״�ִ��
	/*�ڳ�ʼ״̬��Ѳ��Ա������ΪNULL
	*/
	if(SPC_TOR_Inspector[1].SPC == NULL){
		//���SPC��û������SPC�ڵ���ֱ�ӷ��ش��ڳ�ʼ��״̬��Ѳ��Ա��
		//��ֵ������Ϊ�쳣������ж�
		if(SPC_Head->previous == NULL){
			return SPC_TOR_Inspector[1];
		}
		
		//SPC����SPC�ڵ㣬��ô�����һ���ڵ㿪ʼѰ�Һ��ʵ�SPC�ڵ�
		SPC_TOR_Inspector[1].SPC = SPC_Head->previous;
		while(SPC_TOR_Inspector[1].SPC != SPC_Head && SPC_TOR_Inspector[1].SPC->Ring_Active_Amount == 0){
			SPC_TOR_Inspector[1].SPC  = SPC_TOR_Inspector[1].SPC ->previous;
		}
		
		//�������
		//���1����������SPC��û�з����κξ��л�ԾTOR��SPC�����ؿ��Ա�ʶ��Ϊ�쳣�Ľ��
		if(SPC_TOR_Inspector[1].SPC == SPC_Head){
			SPC_TOR_Inspector[1].SPC = NULL;
			SPC_TOR_Inspector[1].TOR = NULL; 
			return SPC_TOR_Inspector[1];
		}
		
		//���2���ҵ��˾�������1����ԾTOR��SPC
		SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].SPC->TOR_Leader->previous;
		/*�˴�����TOR���ڼ������
		*��1����ִ������TOR�������Ĺ����У�û�����񱻽��û��߱�����
		*��2��������������Ҫ���TOR�ڵ�ʱ�����������������������ڴ�˲�������л���ǡ�ý��û��߹�����������
		*/
		do{
			//������ڻ�Ծ̬�򷵻ؽ��
			SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].TOR->previous;
		}while(SPC_TOR_Inspector[1].TOR != SPC_TOR_Inspector[1].SPC->TOR_Leader->previous &&\
				SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_DISABLE_FLAG && \
		TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics) == TASK_OBSTRUCT);
		
		//����TOR���ҽ��
		/*���1���ҵ�һ������Ҫ���tor
		*���2����û���ҵ�tor֮ǰ���tor�˳��˻�Ծ̬���ٴα�����tor���׺��˳�
		*���3���ҵ���tor���������ҵ�֮��û�з��ؾ��˳��˻�Ծ̬
		*/
		if(SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_ENABLE_FLAG && TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics) != TASK_OBSTRUCT){
			//�������1
			return SPC_TOR_Inspector[1];
		}else{
			//���2��3�ϲ�Ϊһ�����������Ѳ��Ա��ֵΪ��ʶ����쳣��NULL
			SPC_TOR_Inspector[1].SPC = NULL;
			SPC_TOR_Inspector[1].TOR = NULL;
			return SPC_TOR_Inspector[1];
		}
		
	
	}
	//���״�ִ��
	
	/*���������ۡ�
	*���1����ͬһ��TOR���о���2�������ϵĽڵ�
	*��1����ǰTOR�ڵ��ڻ���
	*��2����ǰTOR�ڵ��ڻ��У�������Ծ�ڵ�������һ���ڵ�ǰTOR�ڵ��β�ڵ�֮�䡣
	*��3����ǰTOR�ڵ��ڻ��У����ǵ�ǰTOR�ڵ��β�ڵ��в����ڻ�ԾTOR�ڵ�
	*��4����ǰTOR�ڵ���ǻ�β��
	*���2��
	*��1����ǰTOR�����ҽ��е�ǰѡ�е�һ���ڵ㣬������жϹ���ǰ��һ���ڵ㱻���ѣ���ǰ��Ѳ��Ա����Ҫ����
	*/
	
	//���������1��
	if(SPC_TOR_Inspector[1].SPC->Ring_Active_Amount > 1 ){	
		//��һ����ʱ�����洢��ǰѲ��Ա��TORֵ
		TASK_ORBIT_RING* temp_TOR = SPC_TOR_Inspector[1].TOR;
		//��Ѳ��Ա��������TOR����β
		SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].SPC->TOR_Leader->previous;
		//����һ��Ѳ��Ա������һ��TOR�ڵ�ı�־
		unsigned char pass_flag = 0;
		
		do{	
			/*������־�����ζ��Ѳ��Ա�Ѿ�������֮ǰ�Ļ��ͻ�β�м��λ��
			*���ѡ�е�TOR��������״̬�����ڻ�Ծ̬����Է��ص�ǰ�Ľ��
			*������ж�ͨ�������ص��ٽ�㣬��TOR�ڵ㱻������߽��ã�Ѳ��Ա����Ҫ�Դ˸���
			*/
			if(SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_ENABLE_FLAG && pass_flag && TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics)!= TASK_OBSTRUCT){
				return SPC_TOR_Inspector[1];
			}
			/*�˴���������ǰ��TOR�ڵ㣬����pass_flag��־������λ�ò��ܸ��ģ�
			*��������������ǣ������ظ����ص�ǰTOR�ڵ㣬
			*����������˵�ǰTORʱ������pass_flag������0״̬�����Իᱻ����
			*/
			if(SPC_TOR_Inspector[1].TOR == temp_TOR){
				pass_flag = 1;
			}
			SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].TOR->previous;
		}while(SPC_TOR_Inspector[1].TOR != SPC_TOR_Inspector[1].SPC->TOR_Leader->previous);
		
		/*ִ�е��˴���ζ�ŵ�ǰ��TOR����������
		*��1����ǰ��TOR�ͻ�β֮�䲻���ڻ�Ծ��TOR��
		*��2����ǰ��TOR���ڻ�β
		*�����������
		*Ѱ���µ�SPC�ڵ㣬������Ҫ��ת�����������2��
		*/
		goto situation1_flag;
		
		
	}else{
	//���������2��
		situation1_flag:;//�˱�־��if��ת����
		situation2_flag:;//�˱�־���²������ϲ�����ת
		
		//�ӵ�ǰSPC��ͷ�����������
		//���ﲻ�õ�����ѭ�������⣬��Ϊ���������������Ϊ��SPC���бض�������һ��TOR�ǻ�Ծ��
		do{
			if(SPC_TOR_Inspector[1].SPC->next == SPC_Head){
				//�����������SPC���ף����������һ���ڵ�
				SPC_TOR_Inspector[1].SPC = SPC_Head->previous;
				continue;
			}
			SPC_TOR_Inspector[1].SPC = SPC_TOR_Inspector[1].SPC->previous;//�ƶ�SPC�ڵ�
			//�ж��������������л�Ծ�ڵ��SPC��ֹͣ
		}while(SPC_TOR_Inspector[1].SPC->Ring_Active_Amount == 0);
		
		
		//��ǰ�����һ�������˻�ԾTOR��SPC�����Խ�Ѳ��Ա��TOR����Ϊ���ף����Ӵ˴���ʼ������
		SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].SPC->TOR_Leader->previous;
		
		do{
			//�����ǰ��TOR�������Ҳ���������̬
			if(SPC_TOR_Inspector[1].TOR->Task_Enable_Flag == TOR_ENABLE_FLAG && TOR_Status_Get(&SPC_TOR_Inspector[1].TOR->tactics) != TASK_OBSTRUCT){
				/*�˴��жϣ���������һ��TOR�ڵ㿪�������ڻ�Ծ״̬���ɷ��أ�
				*�������֮�󣬸ýڵ���������ص��ٽ�㱻������߱����ã�Ѳ��ԱҲ����Ҫ�Դ˸���
				*/
				return SPC_TOR_Inspector[1];
			}
			SPC_TOR_Inspector[1].TOR = SPC_TOR_Inspector[1].TOR->previous;
		}while(SPC_TOR_Inspector[1].TOR != SPC_TOR_Inspector[1].SPC->TOR_Leader->previous);
		
		/*�������ִ�е��˴�����ζ���ڱ����ڼ䣬��SPC�Ļ�Ծ�ڵ��Ѿ����ٴ��ڻ�Ծ̬��
		*������Ҫ����ѡ��һ��SPC�ڵ㡣
		*/
		goto situation2_flag;
		
	}
}
//��ʽ��ԾѲ��
INSPECTOR Active_Inspector_Static(void){
	return SPC_TOR_Inspector[1];
}
/////////////////////////////////////////////////////////����//////////////////////////////////////////////////////////    


/*************************
* ���ȼ�����
**************************/
///////////////////////////////////////////////////////////��ʼ//////////////////////////////////////////////////////////    

//ָ��������ڵ����ȼ�����
int TOR_Priority_Set(TASK_ORBIT_RING* TOR, unsigned char Priority) {
    if (!TOR)//���TORΪ���򷵻ش���
        return -1;
    if (Priority > 15)//���ȼ����ó������ֵ��������Ϊ���ֵ
        Priority = 15;
    //���ԭ���ȼ�
    TOR->tactics &= 0xf0;
    //д�������ȼ�
    TOR->tactics += Priority;
    return 0;
}
//�鿴ָ��������ڵ�����ȼ�
int TOR_Priority_Get(TASK_ORBIT_RING* TOR) {
    if (!TOR)//���TORΪNULL�����ش�����
        return -1;
    return TOR->tactics & PRIORITY_MASK;
}
///////////////////////////////////////////////////////////����//////////////////////////////////////////////////////////    
/*************************
* ����״̬���û���
**************************/
///////////////////////////////////////////////////////////��ʼ//////////////////////////////////////////////////////////    
//--------------TOR��״̬����
//����״̬
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
//��ȡ״̬
TASK_STATUS TOR_Status_Get(TACTICS* tactics) {
    return (TASK_STATUS)(((*tactics) & STATUS_MASK) >> 6);
}

///////////////////////////////////////////////////////////����//////////////////////////////////////////////////////////    

/*************************
* ��ϣ�����
**************************/
///////////////////////////////////////////////////////////��ʼ//////////////////////////////////////////////////////////    
//��ϣ���ʼ��
void Hash_Init(void) {
    for (unsigned short i = 0; i < HASH_TABLE_SIZE; i++) {
        Hash_TOR.array[i] = NULL;//���е�ֵ��Ϊ��
    }
    Hash_TOR.size = 0;//����Ϊ0��

}
//��ϣ�������(���ɼ�ֵ��)
unsigned int HashFunction(char* Task_Name) {
    unsigned long hash = 5381;
    int c = *Task_Name++;
    while (c) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
		c = *Task_Name++;
    }
    return hash % HASH_TABLE_SIZE;
}
//��ϣ��������
int HashTable_Insert(TASK_ORBIT_RING* TOR) {
    // ͨ����ϣ������ȡ����
    unsigned int index = HashFunction(TOR->Task_name);
    // ����һ���µ�HashNode
#if HEAP_SCHEME == HEAP_SOLUTION_1  
    HashNode* newNode = (HashNode*)hj_malloc(sizeof(HashNode));
#endif

    if (!newNode) {
        return -1;//�ռ�����ʧ��
    }
    newNode->TOR = TOR;
    newNode->next = NULL;

    // ���뵽��ϣ���У������ͻ��ǰ���������
    if (Hash_TOR.array[index] != NULL) {
        newNode->next = Hash_TOR.array[index];
    }
    Hash_TOR.array[index] = newNode;
    //����hash����
    Hash_TOR.size++;
    return 0;
}
//��ϣ��ɾ������
int HashTable_Delete(char* Task_Name) {
    // ͨ����ϣ������ȡ����
    unsigned int index = HashFunction(Task_Name);
    //����������ʱ����
    HashNode* current = Hash_TOR.array[index];
    HashNode* prev = NULL;
    //����hash�ڵ�
    while (current != NULL) {
        if (strcmp(current->TOR->Task_name, Task_Name) == 0) {
            if (prev == NULL) { // ɾ������ͷ�ڵ�
                Hash_TOR.array[index] = current->next;
            }
            else { // ɾ�������м��β�ڵ�
                prev->next = current->next;
            }
#if HEAP_SCHEME == HEAP_SOLUTION_1  
            hj_free(current, sizeof(HashNode));
#endif
            Hash_TOR.size--;
            return 1; // ɾ���ɹ�
        }
        prev = current;
        current = current->next;
    }
    return 0; // δ�ҵ���Ӧ�ڵ�
}
//��ϣ���ѯ����
TASK_ORBIT_RING* HashTable_Find(char* Task_Name) {
    // ͨ����ϣ������ȡ����
    unsigned int index = HashFunction(Task_Name);
    //������ʱ������ȡ��ǰhash�ڵ�
    HashNode* current = Hash_TOR.array[index];
    //�����ڵ�
    while (current != NULL) {
        if (strcmp(current->TOR->Task_name, Task_Name) == 0) {
            return current->TOR;
        }
        current = current->next;
    }
    return NULL; // δ�ҵ�
}
///////////////////////////////////////////////////////////����//////////////////////////////////////////////////////////    
/*************************
* ���ȼ�����������
**************************/
///////////////////////////////////////////////////////////��ʼ//////////////////////////////////////////////////////////    
//���ȼ�����������
//��������ʼ��
int SPC_Init(void) {
#if HEAP_SCHEME == HEAP_SOLUTION_1  
    SPC_Head = hj_malloc(sizeof(SCHEDULING_PRIORITY_CHAIN));
#endif

    if (!SPC_Head)//�ռ�����ʧ��
        return -1;
    SPC_Head->Task_Ring = NULL;
    SPC_Head->Ring_Priority = 0;
    SPC_Head->Ring_Task_Amount = 0;	//SPC�ڵ�����
    SPC_Head->Ring_Active_Amount = 0;//�������ȼ���Ծ����
    SPC_Head->SPC_Type = SPC_TYPE_HEADER;
	SPC_Head->TOR_Leader = NULL;//�������ȼ�TOR��
    SPC_Head->previous = NULL;	//SPC��ĩβ
    SPC_Head->next = NULL;		//SPC������
    return 0;
}
//���
int SPC_Add(unsigned char Priority) {
    //����һ��̽�����ڲ�ѯ����
    SCHEDULING_PRIORITY_CHAIN* SPC_Probe = SPC_Head;

    //��ͷ�ڵ㿪ʼ�����������ҵ���ȷ�Ĳ���λ��
    while (SPC_Probe->next != NULL && SPC_Probe->next->Ring_Priority >= Priority) {
        if (SPC_Probe->next->Ring_Priority == Priority)//�Ѿ����ڸ����ȼ��ڵ�
            return 1;
        SPC_Probe = SPC_Probe->next;
    }
    //�����½ڵ�
#if HEAP_SCHEME == HEAP_SOLUTION_1  
    SCHEDULING_PRIORITY_CHAIN* node = hj_malloc(sizeof(SCHEDULING_PRIORITY_CHAIN));
#endif

    if (!node) //�ռ�����ʧ��
        return -1;

    //��ʼ���½ڵ�
    node->Task_Ring = NULL;
    node->Ring_Priority = Priority;
    node->Ring_Task_Amount = 0;
    node->Ring_Active_Amount = 0;
    node->SPC_Type = SPC_TYPE_GENERAL;
    node->TOR_Leader = NULL;
    node->previous = NULL;
    node->next = NULL;

    //β�ڵ����ͷ������Ϣ��

    //��ǰSPC_Probe��Ҫ����λ�õ�ǰһ���ڵ�
    node->next = SPC_Probe->next;
    if (SPC_Probe->next != NULL) {//�ж�̽�����һ���ڵ��Ƿ�Ϊ��
        SPC_Probe->next->previous = node;//��Ϊ�սڵ�͸��¸ýڵ�ġ�ǰ�ڵ�ָ�롱
    }
    SPC_Probe->next = node;
    node->previous = SPC_Probe;
    //ͷ�����������
    SPC_Head->Ring_Task_Amount++;

    //β�ڵ����ͷ������Ϣ��
    SCHEDULING_PRIORITY_CHAIN* temp = SPC_Head->previous;
    if (SPC_Head->previous == NULL) {
        //�״θ��½ڵ�
        temp = SPC_Head->next;
    }
    while (temp->next != NULL) {
        temp = temp->next;
    }
    SPC_Head->previous = temp;

    return 0;
}
//ɾ��
int SPC_Delete(unsigned char Priority) {
    //����һ��̽�����ڲ�ѯ����
    SCHEDULING_PRIORITY_CHAIN* SPC_Probe = SPC_Head->next;
    //����SPC��
    while (SPC_Probe != NULL) {
        if (SPC_Probe->Ring_Priority == Priority) {//��λ����Ӧ�Ľڵ�
            //�ͷŶѿռ�
            if (SPC_Probe->Ring_Task_Amount > 0) {//�ڵ��������������
                TASK_ORBIT_RING* temp = SPC_Probe->Task_Ring;//����һ����ʱ���������ͷſռ�
                //�Ͽ�ѭ������
                temp->previous->next = NULL;
                while (SPC_Probe->Task_Ring != NULL) {
                    temp = temp->next;
                    #if HEAP_SCHEME == HEAP_SOLUTION_1  
                        //�ͷŶ�ջ�ռ�
                        hj_free(SPC_Probe->Task_Ring->TCB.stack_bottom, SPC_Probe->Task_Ring->TCB.stack_size);
                        //�ͷŽڵ�
                        hj_free(SPC_Probe->Task_Ring, sizeof(TASK_ORBIT_RING));
                    #endif
                    //����SPC�м�¼������
                    SPC_Probe->Ring_Task_Amount--;
                    //���½ڵ�
                    SPC_Probe->Task_Ring = temp;
                }
            }
            //ɾ���������Ľڵ�
            //��������̽ͷ�պ�ָ���˼����ͷŵĽڵ�
            if (SPC_Detector == SPC_Probe) {
                //���1������ֻ��һ���ڵ㣬��ô̽ͷ�ص�ͷ��㣨�ƶ�����һ���ڵ㣩
                //���2�������ж���ڵ㣬���Ǳ�ɾ���Ľڵ������ȼ���ߵĽڵ㣬��ô̽ͷ�ƶ�����һ���ڵ�
                //���3�������ж���ڵ㣬��ɾ���������ȼ���͵Ľڵ㣬��ô̽ͷ�ƶ�����һ���ڵ�
                //���4�������ж���ڵ㣬��ɾ���Ľڵ����м�ڵ㣬��ô̽ͷ�ƶ�����һ���ڵ�
                if (SPC_Detector->previous == SPC_Head && SPC_Head->Ring_Task_Amount > 1) {//���2
                    SPC_Detector = SPC_Detector->next;//̽ͷָ����һ���ڵ�
                }
                else {
                    SPC_Detector = SPC_Detector->previous;//̽ͷ���ƣ�
                }

            }
            //ɾ��̽��ָ��Ľڵ�
            if (SPC_Probe->next != NULL) {//̽����һ���ڵ㲻Ϊ��
                SPC_Probe->next->previous = SPC_Probe->previous;
            }
            SPC_Probe->previous->next = SPC_Probe->next;
            //�ͷŽڵ�
            #if HEAP_SCHEME == HEAP_SOLUTION_1  
                hj_free(SPC_Probe, sizeof(SCHEDULING_PRIORITY_CHAIN));
            #endif

            //�����������Լ�
            --SPC_Head->Ring_Task_Amount;
            return 0;
        }
        SPC_Probe = SPC_Probe->next;
    }
    return -1;//SPC���в����ڸ����ȼ��ڵ�
}
//��ȡSPC_Head��ֵ
SCHEDULING_PRIORITY_CHAIN* Get_SPC_Head() {
    return SPC_Head;
}
//��ȡSPC_Detector��ֵ
SCHEDULING_PRIORITY_CHAIN* Get_SPC_Detector() {
    return SPC_Detector;
}
//��ȡָ�����ȼ��ĵ������ڵ�
SCHEDULING_PRIORITY_CHAIN* Get_Designated_SPC(unsigned char Priority) {
    SCHEDULING_PRIORITY_CHAIN* temp = SPC_Head->next;
    while (temp != NULL) {
        if (temp->Ring_Priority == Priority)
            return temp;
        temp = temp->next;
    }
    return NULL;
}
//��ѯ�Ƿ����ָ�����ȼ��ĵ������ڵ�
int SPC_Verify(unsigned char Priority) {
    if (!SPC_Head->next) {//������Ϊ��
        return -1;
    }
    SCHEDULING_PRIORITY_CHAIN* temp = SPC_Head->next;
    while (temp != NULL) {
        if (temp->Ring_Priority == Priority)
            return 1;//����
        temp = temp->next;
    }
    return 0;//������
}

///////////////////////////////////////////////////////////����//////////////////////////////////////////////////////////


/*************************
* ������������
**************************/
//������������
///////////////////////////////////////////////////////////��ʼ//////////////////////////////////////////////////////////

//���һ��TOR�ڵ�(!!!!!!!!!!!ע�⣬ֻ�������һ��TOR����û���ڹ�ϣ������ӽڵ㣬Ŀǰ�滮��δ����Task_Create�н������߽��)
int TOR_Create(TASK_FUNCTION taskFunction, \
    void* Args, \
    unsigned char Priority, \
    unsigned char* icon, \
    char* Task_name, \
    StackDepth size, \
    TASK_ORBIT_RING** node) {
    //������ȼ�
    if (Priority > 15)
        Priority = 15;
    //���һ��ָ�����ȼ���SPC
    if (SPC_Add(Priority) == -1) {
        return -1;//��ӦSPC����ʧ��
    }
    //��ȡָ��SPC�ڵ�
    SCHEDULING_PRIORITY_CHAIN* temp = Get_Designated_SPC(Priority);


    //����TOR�ڵ�
    #if HEAP_SCHEME == HEAP_SOLUTION_1  
        (*node) = hj_malloc(sizeof(TASK_ORBIT_RING));
    #endif


    if (!(*node)) {
        return -2;//TOR����ʧ��
    }
    //TOR��ʼ��������������������������������������������������������ʼ
    //TCB��ʼ����������ռ䣬ֻ���ʼ��
    (*node)->TCB.taskFunction = taskFunction;
    (*node)->TCB.Args = Args;
    (*node)->TCB.stack_bottom = NULL;
    (*node)->TCB.stack_size = size;
    (*node)->TCB.stack_top = NULL;
    //TOR��������
    (*node)->self_SPC = temp;//����SPC
    (*node)->Task_Enable_Flag = TOR_DISABLE_FLAG;//������
    (*node)->icon = icon;
    (*node)->self_handle = (*node);
    (*node)->tactics = Priority;//�����ֶ�Ϊ0�����ȼ��ֶθ�ֵ
    (*node)->next = NULL;
    (*node)->previous = NULL;
    //�������Ƴ�ʼ��

    strcpy((*node)->Task_name, Task_name);
    //TOR��ʼ��-----------------------------------------------------����

    //TOR�뻷
    //���һ����ǰSPCû�й�������
    if (temp->Ring_Task_Amount == 0) {
        temp->Task_Ring = (*node);
        temp->Task_Ring->next = (*node);
        temp->Task_Ring->previous = (*node);
        //����SPC�е�TOR���ױ��
        temp->TOR_Leader = (*node);
    }
    else {
        //�������SPC����������.��ʱTOR���뵽SPC���ص��β������
        //TOR�����ڻ���
        (*node)->next = temp->TOR_Leader;
        (*node)->previous = temp->TOR_Leader->previous;
        //���뻷��
        (*node)->next->previous = (*node);
        (*node)->previous->next = (*node);
    }
    ++temp->Ring_Task_Amount;//���ӽڵ�����
    return 0;
}

//TOR�ƶ������ȼ����ָı�󣬴ӵ�ǰSPC�ƶ���ָ�����ȼ���SPC��
int TOR_Move(TASK_ORBIT_RING* TOR) {
    //���ڵ��Ƿ�Ϊ��
    if (!TOR) {
        return -1;//���Ϊ�շ��ش���
    }
    //���1����SPC�����ڽڵ�
    //ȷ��Ŀ�����ȼ���SPC�Ƿ����a(���ڻ��ߴ����ɹ�������ִ��)
    if (SPC_Add(TOR_Priority_Get(TOR->self_handle)) == -1) {
        //����ʧ��
        return -1;
    }
    //����Ŀ��TOR
    TOR->next->previous = TOR->previous;
    TOR->previous->next = TOR->next;
    //��ȡĿ��SPC���
    SCHEDULING_PRIORITY_CHAIN* temp = Get_Designated_SPC(TOR_Priority_Get(TOR->self_handle));
    if (temp->Ring_Task_Amount == 0) {
        //������¿��ٵ�SPC�ڵ�
        temp->Ring_Task_Amount++;
        temp->TOR_Leader = TOR;
        temp->Task_Ring = TOR;
        TOR->next = TOR;
        TOR->previous = TOR;
        return 0;
    }
    //TOR������TOR����
    TOR->next = temp->TOR_Leader;
    TOR->previous = temp->TOR_Leader->previous;
    //�뻷
    TOR->next->previous = TOR;
    TOR->previous->next = TOR;
    temp->Ring_Task_Amount++;
    return 0;
}

//---------------TOR���������
//����ָ��TOR
int TOR_Enable(TASK_ORBIT_RING* TOR) {
    if (!TOR) {
        return -1;
    }
    //��֤���TOR�Ƿ��Ѿ�����
    if (TOR->Task_Enable_Flag == TOR_ENABLE_FLAG) {
        //�����TOR�Ѿ�������ֱ�ӷ���
        return 0;
    }
    #if HEAP_SCHEME == HEAP_SOLUTION_1  
        TOR->TCB.stack_bottom = (stack_t*)hj_malloc(TOR->TCB.stack_size*sizeof(stack_t));//����ջ�ײ������ٿռ�
    #endif
    if (!TOR->TCB.stack_bottom) {
        return -1;//��ջ�ռ�����ʧ��
    }
    TOR->TCB.stack_top = TOR->TCB.stack_bottom + TOR->TCB.stack_size;//����ջ��
    *TOR->TCB.stack_bottom = STACK_DETECTION_VALUE;//ջ������һ���ض�ֵ
    TOR->Task_Enable_Flag = TOR_ENABLE_FLAG;
    return 0;
}
//����ָ��TOR
int TOR_Disable(TASK_ORBIT_RING* TOR) {
    /*�����������Ĺ��̣�Ӧ����֤û����ԭ���Եġ�
   *����ջ���ͷź�����ں��������ȣ���ô�����������֡������ѹջ�����쳣����ϵͳ������
   */
    if (!TOR) {
        return -1;
    }
    //�ر�����ȫ���ж�(�������������������´�������ֲ����)
    __disable_irq(); // �ر�ȫ���ж�

    //��֤���TOR�Ƿ�����
    if (TOR->Task_Enable_Flag == TOR_DISABLE_FLAG) {
        //���û��������ֱ�ӷ���
        //��������ȫ���жϣ��������������������´�������ֲ������
    __enable_irq(); // ����ȫ���ж�
        return 0;
    }
    #if HEAP_SCHEME == HEAP_SOLUTION_1  
        hj_free(TOR->TCB.stack_bottom, TOR->TCB.stack_size*sizeof(stack_t));//�ͷ�ָ��TOR�Ķ�ջ�ռ�
    #endif
    TOR->TCB.stack_bottom = NULL;
    TOR->TCB.stack_top = NULL;
    TOR->Task_Enable_Flag = TOR_DISABLE_FLAG;//�ر����ñ�־
    //�����ǰ��״̬���ڻ�Ծ̬����ô��SPC�Ļ�Ծ̬��������һ��
    if (TOR_Status_Get(&TOR->tactics) != TASK_OBSTRUCT) {
        TOR->self_SPC->Ring_Active_Amount--;
    }
    //��������ȫ���жϣ��������������������´�������ֲ������
    __enable_irq(); // ����ȫ���ж�
    return 0;
}
//��ջ������
Stack_Overflow_t TOR_Stack_Overflow_Detection(TASK_ORBIT_RING* TOR) {
    int detection_result = TOR->TCB.stack_top - TOR->TCB.stack_bottom;
    //���ջ��Ĭ��ֵ�Ƿ�ı�
    if (*TOR->TCB.stack_bottom != STACK_DETECTION_VALUE){
        detection_result = -1;
	}
    //�жϷ���    
    if (detection_result < 0) {
        return (Stack_Overflow_t)-1;//ջ���
    }
    else if (detection_result == 0) {
        return (Stack_Overflow_t)1;//����ٽ��
    }
    else {
        return (Stack_Overflow_t)0;//��ȫ
    }
}
//�������ȼ�������Ѿ����õ�һ��TOR
TASK_ORBIT_RING* TOR_Lowest_Enable_Find() {
    //����һ����ʱSPCָ��
    SCHEDULING_PRIORITY_CHAIN* temp_spc = SPC_Head->previous;
    if (!temp_spc) {
        return NULL;//SPC��Ϊ��
    }
    //��ʼ�������SPC��
    while (temp_spc != SPC_Head) {
        //����һ����ʱTORָ��
        TASK_ORBIT_RING* temp_tor = temp_spc->TOR_Leader;
        //����TOR��Ѱ�ҹ��������
        do {
            //��֤�Ƿ�������Ѿ����õ�����
            if (temp_tor->Task_Enable_Flag == TOR_ENABLE_FLAG) {
                //�ҵ�����Ҫ�������,
                return temp_tor;
            }
            temp_tor = temp_tor->next;
        } while (temp_tor != temp_spc->TOR_Leader);
    }
    return NULL;//�쳣��SPC�����������õ�����ͨ�������ܣ�
}
//�������ȼ�������Ѿ������Ҵ���ָ��״̬��TOR
TASK_ORBIT_RING* TOR_Lowest_Enable_Designated_Status_Find(TASK_STATUS status){
    //����һ����ʱSPCָ��
    SCHEDULING_PRIORITY_CHAIN* temp_spc = SPC_Head->previous;
    if (!temp_spc) {
        return NULL;//SPC��Ϊ��
    }
    TASK_ORBIT_RING* temp_tor = NULL;
	
	//���Ҫ�ҵĽڵ��ǹ���̬
	if(status == TASK_OBSTRUCT){
		//���Ҫ�ҵĽڵ��ǹ���̬
		while(temp_spc != SPC_Head){
			//����һ����ʱTORָ��
			temp_tor = temp_spc->TOR_Leader;
			do{
				if(temp_tor->Task_Enable_Flag == TOR_ENABLE_FLAG && TOR_Status_Get(&temp_tor->tactics) == TASK_OBSTRUCT){
					return temp_tor;
				}
				temp_tor = temp_tor->next;
			} while (temp_tor != temp_spc->TOR_Leader);
			temp_spc = temp_spc->previous;
		}
		//����ѭ������SPC����������û���������ʷ��ش���ֵ
	}else if(status == TASK_READY){
		//���Ҫ�ҵĽڵ��Ǿ���̬
		//��ʼ�������SPC��
		while(temp_spc != SPC_Head && temp_spc->Ring_Active_Amount == 0 ){
			temp_spc = temp_spc->previous;
		}
		//��������ھ��л�Ծ��SPC�ڵ㣬���ش���
		if(temp_spc == SPC_Head){
			return NULL;
		}
		//����һ����ʱTORָ��
		temp_tor = temp_spc->TOR_Leader;
		
		do{
			if(temp_tor->Task_Enable_Flag == TOR_ENABLE_FLAG && TOR_Status_Get(&temp_tor->tactics) == TASK_READY){
				return temp_tor;
			}
			temp_tor = temp_tor->next;
		} while (temp_tor != temp_spc->TOR_Leader);
			
	}		
    return NULL;//�쳣��SPC��������ָ��Ҫ�������ͨ�������ܣ�
}
///////////////////////////////////////////////////////////����//////////////////////////////////////////////////////////

/******************
* �����л�
*******************/
///////////////////////////////////////////////////////////��ʼ//////////////////////////////////////////////////////////

//�ڵ�ǰ����Ѱ�ҿ�ִ�е�����
/*
* 1���ڵ�ǰ���п������������£�
* ��1����ǰ�����ھ��л�Ծ���񻷵�������ȼ�SPC�ڵ㣬��ѭ��������ǰTOR���Ľڵ�
* ��2�����ȵ�ǰ���������ȼ���SPC�����������ѻ����������õ�����£���ת����SPC�ڵ�
* 2���ڵ�ǰ�������ڿ������������£�
* ��1�����ȵ�ǰ���������ȼ���SPC�����������ѻ����������õ�����£���ת����SPC�ڵ�
* ��2����������ڸ������ȼ���SPC���ֻ�Ծ̬������ô���±�����Ѱ�ҿ�ִ�е�SPC�ڵ�
* ��3��SPC�������ڿ�ִ������ʱ��ִ�п�������
* -------------
* Ŀǰ���뷨������һ����־λ�������־λ�ı�ͼ��SPC������ִ�е�ǰSPC��
*/

//���ݲ��ԣ�ѡ�������л���ʽ
#if SPECIAL_PRIORITY_CONFIG == SPECIAL_PRIORITY_DISABLE
void Task_Switch(void) {
    //�������1�������״�ִ�У���ʱ̽ͷָ��NULL������ֻ��һ��ָ��NULL�Ŀ��ܣ�֮���ڲ������쳣�������ֻ������Task_Idle����ͨ����֮���л���
    if (SPC_Detector == NULL) {
        //��̽ͷָ���һ��SPC
        SPC_Detector = SPC_Head->next;
        if (SPC_Detector == NULL) {
            //ִ�п����������
            /*
            * ��ʱ����׸�SPC�����ڣ���ô��ʱϵͳ�����쳣��
            * Ϊ��Ӧ���쳣�������SPCִ�п�������
            */
            SPC_Detector = &SPC_Idle;
            return;
        }
        //����SPC��Ѱ�Һ��ʵ�SPC,��̽ͷû�б�����SPC��β�������£�ֻҪ�����˲����ڻ�Ծ�ڵ��SPCʱ���������������
        while (SPC_Detector != NULL && SPC_Detector->Ring_Active_Amount == 0) {
            SPC_Detector = SPC_Detector->next;
        }
        //�������1��
        if (SPC_Detector == NULL) {
            /*
            * ��ʱ��̽ͷ������β��Ҳ����ζ��SPC�������ڿ���ִ�е�����
            * Ϊ��Ӧ���쳣��Ӧ��ִ�п�������
            */
            //�����������
            SPC_Detector = &SPC_Idle;
            return;
        }
        /*�������2�����ҵ��˺��ʵ�����
        * ������������Ϊ����̬������
        */
		//���˵�û�����õ�����ʹ��ڹ���̬������
		while(SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT){
			SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
		}
        //ѡ����������Ϊ����̬
        TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_RUNTIME);
        return;
    }
	
	//ջ������(������һ��������ֹ�Ŀ��ܣ�����ֹ���ټ�������δ������Ϊ������ɸѡ������������״̬��TOR)
	if(SPC_Detector->Task_Ring->Task_Enable_Flag){
		STACK_OVERFLOW_FLAG = TOR_Stack_Overflow_Detection(SPC_Detector->Task_Ring);
	}
	
	
	/*����������ȴ����С�
	*�����еĶ�ʱ����������ʱ���Լ������ʱ��Ϊ0�������̻��Ѹ�����
	*/
	Synchronization_Primitives_Update();
    //������һ�����롾�����3�������ڿ�������ʱ���������л�

    //�����ǰ������ִ�еĿ�������
    if (SPC_Detector == &SPC_Idle) {
        //���1�����������񱻻��ѻ��߳���
        if (TaskQueueUpdated == NEW_TASK_AVAILABLE) {
            SPC_Detector = SPC_Head->next;//����ǰ̽ͷ����Ϊ�׸�SPC
            //��������SPC����Ѱ��������
            while (SPC_Detector != NULL && SPC_Detector->Ring_Active_Amount == 0) {
                SPC_Detector = SPC_Detector->next;
            }
            //��ʱ��SPC_DetectorΪ�������SPC�ڵ�
			if(SPC_Detector == NULL){
				//�������û����������ô���½����������
				//�������־����
				TaskQueueUpdated = NEW_TASK_NOT_AVAILABLE;
				SPC_Detector = &SPC_Idle;
				return;
			}
			//Ѱ�ҵ���ִ�е�TOR�ڵ�			
			while(SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT){
				SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
			}
            //�������־����
            TaskQueueUpdated = NEW_TASK_NOT_AVAILABLE;
        }
		return;
    }



    // �������2�������״�ִ�������л�
    // 
    //��鵱ǰ�����Ƿ�����Ϊ�˹���̬
    if (TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) != TASK_OBSTRUCT) {
        //�����ǰ�����ǻ�Ծ̬����ô�Ϳ������õ�ǰ��������Ϊ����̬
        TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_READY);
    }

    /*����������־λ�Ƿ񱻸ı�
    * ��1�������־λ��ʾû����������룺
    * ��1�������ǰ�����ڿ����������ڵ�ǰ�����б���
    * ��2�������ǰ�������ڿ���������ô̽ͷ����β���ķ������
    * ��2�������־λ��ʾ����������룺������һ����ʱָ�볯��SPCͷ���ķ������
    * ��1������������������ڸ����ȼ�SPC����ô���̽�̽ͷ�ƶ��������ȼ�SPC
    * ��2�������������ͷ���Ҳû�з��ֻ�Ծ������ô���̻ص���ǰ��SPC�ڵ㰴�������1����ʼ����
    */
    
    if (TaskQueueUpdated == NEW_TASK_NOT_AVAILABLE) {
    not_available:;//�����ת��ǩΪ�����2���е���ת��־
        //�������־λû�г��ָı�
         //�жϵ�ǰTOR�����Ƿ��л�Ծ��TOR�ڵ�
        if (SPC_Detector->Ring_Active_Amount == 0) {
            //��ǰTOR�������ڿ����еĽڵ�
            
            //Ѱ��һ�����Ծ��л�ԾTOR��SPC
            do {
                SPC_Detector = SPC_Detector->next;
            } while (SPC_Detector != NULL && SPC_Detector->Ring_Active_Amount == 0);


            /*
            * �жϷ��صĽ������������֣�
            * ��1������һ��SPC��Ѱ�ҵ��˷���Ҫ���SPC
            * ��2������NULL��û�п�ִ�е�����
            */

            if (!SPC_Detector) {
                //�����1��������ؽ��Ϊ�գ�
                /*
                * �����������...
                */
                SPC_Detector = &SPC_Idle;
                return;
            }
            //�����2��������SPC,�������SPC��һ��������һ�����õ�TOR�����Բ���������ѭ��
            while (SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT) {
                SPC_Detector = SPC_Detector->next;
            }
            //������ɺ󣬴�ʱ��SPC_DetectorΪ������Ҫִ�е�����
        }
        else {//��ǰ�����ڿ����нڵ�
            //SPC��̽��ǰ�ƶ�,�����⵽δ���õ�������߱���������������������ֱ���ҵ�һ����Ч������
            do {
                SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
            } while (SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT);
        }
    }
    else {
        //���½���־λ����Ϊ��û��������
        TaskQueueUpdated = NEW_TASK_NOT_AVAILABLE;
        //�������־λ�����˸ı�
        SCHEDULING_PRIORITY_CHAIN* temp_task_detector = SPC_Detector->previous;//����һ����ʱSPC̽ͷ��ǰ����
        while (temp_task_detector != SPC_Head) {
            //����ҵ�һ������Ҫ���SPC�����˳�
            if (temp_task_detector->Ring_Active_Amount > 0) {
                break;
            }
            temp_task_detector = temp_task_detector->previous;
        }
        //���������
        /*
        * ��1���ҵ��˷���Ҫ���SPC��
        * ��2��������ͷ��㣬Ҳ����ǰ���SPC��û��TOR����
        */
        if (temp_task_detector == SPC_Head) {
            //�����ʱ��SPCǰ����û�д��ڻ�Ծ��TOR����ô��ת��ԭ����SPC�����������1������
            goto not_available;
        }

        //�ҵ��˷���Ҫ��Ľڵ㣬����ʱ̽ͷ��ֵ��ֵ��̽ͷ
        SPC_Detector = temp_task_detector;

        //�����2��������SPC,�������SPC��һ��������һ�����õ�TOR�����Բ���������ѭ��
        while (SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT) {
            SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
        }
        //������ɺ󣬴�ʱ��SPC_DetectorΪ������Ҫִ�е�����


    }
    //ѡ����������Ϊ����̬
    TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_RUNTIME);
	return;
}
#else
//unsigned char Special_Priority_Count = 0;//����һ��������¼�������ȼ���TOR��������ΪSPC_Head�������Ա��������¼SPC��������
SCHEDULING_PRIORITY_CHAIN* SPC_Standard = NULL;//��һ��������¼��׼�����ȼ�̽ͷ��λ��
//���ݵ�ǰֵ�ж�Ӧ��ִ���������ȼ����Ǳ�׼���ȼ�
unsigned char Priority_Switch_Tactics = 0;//��׼���ȼ�Ϊ0���������ȼ�Ϊ1
void Task_Switch(void){
	 //�������1�������״�ִ�У���ʱ̽ͷָ��NULL������ֻ��һ��ָ��NULL�Ŀ��ܣ�֮���ڲ������쳣�������ֻ������Task_Idle����ͨ����֮���л���
    if (SPC_Detector == NULL) {
        //��̽ͷָ���һ��SPC
        SPC_Detector = SPC_Head->next;
        if (SPC_Detector == NULL) {
            //ִ�п����������
            /*
            * ��ʱ����׸�SPC�����ڣ���ô��ʱϵͳ�����쳣��
            * Ϊ��Ӧ���쳣�������SPCִ�п�������
            */
            SPC_Detector = &SPC_Idle;
			//��¼��ǰ��SPCλ��
			SPC_Standard = SPC_Detector;
			//�л�ִ�в���
			Priority_Switch_Tactics = 1;
            return;
        }
        //����SPC��Ѱ�Һ��ʵ�SPC,��̽ͷû�б�����SPC��β�������£�ֻҪ�����˲����ڻ�Ծ�ڵ��SPCʱ���������������
        while (SPC_Detector != NULL && SPC_Detector->Ring_Active_Amount == 0) {
            SPC_Detector = SPC_Detector->next;
        }
        //�������1��
        if (SPC_Detector == NULL) {
            /*
            * ��ʱ��̽ͷ������β��Ҳ����ζ��SPC�������ڿ���ִ�е�����
            * Ϊ��Ӧ���쳣��Ӧ��ִ�п�������
            */
            //�����������
            SPC_Detector = &SPC_Idle;
			//��¼��ǰ��SPCλ��
			SPC_Standard = SPC_Detector;
			//�л�ִ�в���
			Priority_Switch_Tactics = 1;
            return;
        }
        /*�������2�����ҵ��˺��ʵ�����
        * ������������Ϊ����̬������
        */
		//���˵�û�����õ�����ʹ��ڹ���̬������
		while(SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT){
			SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
		}
		
        //ѡ����������Ϊ����̬
        TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_RUNTIME);
		//��¼��ǰ��SPCλ��
		SPC_Standard = SPC_Detector;
		//�л�ִ�в���
		Priority_Switch_Tactics = 1;
        return;
    }
	
	//ջ������(������һ��������ֹ�Ŀ��ܣ�����ֹ���ټ�������δ������Ϊ������ɸѡ������������״̬��TOR)
	if(SPC_Detector->Task_Ring->Task_Enable_Flag){
		STACK_OVERFLOW_FLAG = TOR_Stack_Overflow_Detection(SPC_Detector->Task_Ring);
	}
	
	
	/*����������ȴ����С�
	*�����еĶ�ʱ����������ʱ���Լ������ʱ��Ϊ0�������̻��Ѹ�����
	*/
	Synchronization_Primitives_Update();
    //������һ�����롾�����3�������ڿ�������ʱ���������л�

	tactics_select:;//����ѡ����ת��־
	
	//�����ǰ������ִ�б�׼���ȼ�------------------------
	if(Priority_Switch_Tactics == 0){
		SPC_Detector = SPC_Standard;//��֮ǰ��¼�ı�׼���ȼ�λ�ø�ֵ��̽ͷ
		//�����ǰ������ִ�еĿ�������
		if (SPC_Detector == &SPC_Idle) {
			//���1�����������񱻻��ѻ��߳���
			if (TaskQueueUpdated == NEW_TASK_AVAILABLE) {
				SPC_Detector = SPC_Head->next;//����ǰ̽ͷ����Ϊ�׸�SPC
				//��������SPC����Ѱ��������
				while (SPC_Detector!=NULL && SPC_Detector->Ring_Active_Amount == 0) {
					SPC_Detector = SPC_Detector->next;
				}
				//��ʱ��SPC_DetectorΪ�������SPC�ڵ�
				//�����ʱ��SPCû�нڵ㣬��ô����ʹ�ÿ�������
				if(SPC_Detector == NULL){
					//�������־����
					TaskQueueUpdated = NEW_TASK_NOT_AVAILABLE;
					SPC_Detector = &SPC_Idle;
					//��¼��ǰ��SPCλ��
					SPC_Standard = SPC_Detector;
					//�л�ִ�в���
					Priority_Switch_Tactics = 1;
					return;
				}
				//Ѱ�ҵ���ִ�е�TOR�ڵ�			
				while(SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT){
					SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
				}
				//�������־����
				TaskQueueUpdated = NEW_TASK_NOT_AVAILABLE;
			}
			//��¼��ǰ��SPCλ��
			SPC_Standard = SPC_Detector;
			//�л�ִ�в���
			Priority_Switch_Tactics = 1;
			return;
		}



		// �������2�������״�ִ�������л�
		// 
		//��鵱ǰ�����Ƿ�����Ϊ�˹���̬
		if (TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) != TASK_OBSTRUCT) {
			//�����ǰ�����ǻ�Ծ̬����ô�Ϳ������õ�ǰ��������Ϊ����̬
			TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_READY);
		}

    /*����������־λ�Ƿ񱻸ı�
    * ��1�������־λ��ʾû����������룺
    * ��1�������ǰ�����ڿ����������ڵ�ǰ�����б���
    * ��2�������ǰ�������ڿ���������ô̽ͷ����β���ķ������
    * ��2�������־λ��ʾ����������룺������һ����ʱָ�볯��SPCͷ���ķ������
    * ��1������������������ڸ����ȼ�SPC����ô���̽�̽ͷ�ƶ��������ȼ�SPC
    * ��2�������������ͷ���Ҳû�з��ֻ�Ծ������ô���̻ص���ǰ��SPC�ڵ㰴�������1����ʼ����
    */
    
		if (TaskQueueUpdated == NEW_TASK_NOT_AVAILABLE) {
		not_available:;//�����ת��ǩΪ�����2���е���ת��־
			//�������־λû�г��ָı�
			 //�жϵ�ǰTOR�����Ƿ��л�Ծ��TOR�ڵ�
			if (SPC_Detector->Ring_Active_Amount == 0) {
				//��ǰTOR�������ڿ����еĽڵ�
				
				//Ѱ��һ�����Ծ��л�ԾTOR��SPC
				do {
					SPC_Detector = SPC_Detector->next;
				} while (SPC_Detector != NULL && SPC_Detector->Ring_Active_Amount == 0);


				/*
				* �жϷ��صĽ������������֣�
				* ��1������һ��SPC��Ѱ�ҵ��˷���Ҫ���SPC
				* ��2������NULL��û�п�ִ�е�����
				*/

				if (!SPC_Detector) {
					//�����1��������ؽ��Ϊ�գ�
					/*
					* �����������...
					*/
					SPC_Detector = &SPC_Idle;
					//��¼��ǰ��SPCλ��
					SPC_Standard = SPC_Detector;
					//�л�ִ�в���
					Priority_Switch_Tactics = 1;
					return;
				}
				//�����2��������SPC,�������SPC��һ��������һ�����õ�TOR�����Բ���������ѭ��
				while (SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT) {
					SPC_Detector = SPC_Detector->next;
				}
				//������ɺ󣬴�ʱ��SPC_DetectorΪ������Ҫִ�е�����
			}
			else {//��ǰ�����ڿ����нڵ�
				//SPC��̽��ǰ�ƶ�,�����⵽δ���õ�������߱���������������������ֱ���ҵ�һ����Ч������
				do {
					SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
				} while (SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT);
			}
		}
		else {
			//���½���־λ����Ϊ��û��������
			TaskQueueUpdated = NEW_TASK_NOT_AVAILABLE;
			//�������־λ�����˸ı�
			SCHEDULING_PRIORITY_CHAIN* temp_task_detector = SPC_Detector->previous;//����һ����ʱSPC̽ͷ��ǰ����
			while (temp_task_detector != SPC_Head) {
				//����ҵ�һ������Ҫ���SPC�����˳�
				if (temp_task_detector->Ring_Active_Amount > 0) {
					break;
				}
				temp_task_detector = temp_task_detector->previous;
			}
			//���������
			/*
			* ��1���ҵ��˷���Ҫ���SPC��
			* ��2��������ͷ��㣬Ҳ����ǰ���SPC��û��TOR����
			*/
			if (temp_task_detector == SPC_Head) {
				//�����ʱ��SPCǰ����û�д��ڻ�Ծ��TOR����ô��ת��ԭ����SPC�����������1������
				goto not_available;
			}

			//�ҵ��˷���Ҫ��Ľڵ㣬����ʱ̽ͷ��ֵ��ֵ��̽ͷ
			SPC_Detector = temp_task_detector;

			//�����2��������SPC,�������SPC��һ��������һ�����õ�TOR�����Բ���������ѭ��
			while (SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT) {
				SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
			}
			//������ɺ󣬴�ʱ��SPC_DetectorΪ������Ҫִ�е�����
		}
		//ѡ����������Ϊ����̬
		TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_RUNTIME);
		//��¼��ǰ��SPCλ��
		SPC_Standard = SPC_Detector;
		//�л�ִ�в���
		Priority_Switch_Tactics = 1;
		return;
	}else{
		//�����ǰ���������ȼ�
		SPC_Detector = SPC_Head;//����ʽͷ�ڵ��ֵ��ֵ��̽ͷ
		
		//�������ͷ�ڵ��Ƿ���л�Ծ������
		if(SPC_Detector->Ring_Active_Amount == 0){
			//���ͷ���û�л�Ծ��������������ת����׼���ȼ�
			Priority_Switch_Tactics = 0;//����ִ�в���
			goto tactics_select;
		}
		
		//���˵�û�л�Ծ����������
		//��鵱ǰ�����Ƿ�����Ϊ�˹���̬
		if (TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) != TASK_OBSTRUCT) {
			//�����ǰ�����ǻ�Ծ̬����ô�Ϳ������õ�ǰ��������Ϊ����̬
			TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_READY);
		}
		
		//������ɵ�ǰ�����Ѱ����һ������,���ڼ����һ�����л�Ծ�������Բ���������ѭ��
		do{
			SPC_Detector->Task_Ring = SPC_Detector->Task_Ring->next;
			//ֻҪĿ�괦�ڲ�����״̬���߹���̬�����̼�������
		}while(SPC_Detector->Task_Ring->Task_Enable_Flag == TOR_DISABLE_FLAG || TOR_Status_Get(&SPC_Detector->Task_Ring->tactics) == TASK_OBSTRUCT);
		
		//��ȡ���µķ���Ҫ��Ľڵ��,��������Ϊ����̬���̷���
		//ѡ����������Ϊ����̬
		TOR_Status_Set(&SPC_Detector->Task_Ring->tactics, TASK_RUNTIME);
		//�޸����ȼ�����
		Priority_Switch_Tactics = 0;
		return;
	}
}
#endif
/*************************
* ��������
**************************/
//��������
///////////////////////////////////////////////////////////��ʼ//////////////////////////////////////////////////////////
//����������
void Task_Idle(void* args) {
	while(1);
}
//���������ʼ��
void Task_Idle_Init(void) {
    //SPC_Idle��ʼ��
    SPC_Idle.Ring_Priority = 0;
    SPC_Idle.next = NULL;
    SPC_Idle.previous = NULL;
    SPC_Idle.Ring_Active_Amount = 1;
    SPC_Idle.Ring_Task_Amount = 1;
    SPC_Idle.SPC_Type = SPC_TYPE_GENERAL;
    SPC_Idle.Task_Ring = &TOR_Idle;
    SPC_Idle.TOR_Leader = &TOR_Idle;
    //TOR_Idle��ʼ��
    TOR_Idle.icon = NULL;
    TOR_Idle.next = NULL;
    TOR_Idle.previous = NULL;
    TOR_Idle.self_handle = &TOR_Idle;
    TOR_Idle.self_SPC = &SPC_Idle;
    TOR_Idle.tactics = 0;
    TOR_Idle.Task_Enable_Flag = TOR_ENABLE_FLAG;
    strcpy(TOR_Idle.Task_name, "Task_Idle");
    //TCB����
    TOR_Idle.TCB.Args = NULL;
    TOR_Idle.TCB.stack_bottom = &Task_Idle_Stack[0];
    TOR_Idle.TCB.stack_size = TASK_IDLE_STACK_SIZE;
    TOR_Idle.TCB.stack_top = &Task_Idle_Stack[TOR_Idle.TCB.stack_size];
    TOR_Idle.TCB.taskFunction = Task_Idle;
    //TCB��ʼ��

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

///////////////////////////////////////////////////////////����//////////////////////////////////////////////////////////


//ʱ���ж�ִ��ISR
void SysTick_Handler(void) {
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}


/*************************
* ����API�ӿ�
*************************/
/**�����ں�����**/
//-----------------------------------------------------------�����ں����� ��ʼ------------------------------------------------

//ʱ��Ƭ����
//ϵͳ�δ�ʱ����ʼ������ʱ�жϣ���������������ͨ����
void SysTick_init(void) {
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;//��ʱ�����ã�Ĭ�ϲ���Ƶ��
	SysTick->VAL   = 0;
	SysTick->LOAD  = SYSTICK_LOAD;//ϵͳ��װֵ��
	NVIC_SetPriority(SysTick_IRQn, 0x02);
}

//�����ں˳�ʼ��
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

//����ϵͳ����
void Inkos_StartUp(void){
	Task_Switch();
	__asm {//�û���ջָ���ʼ��
		MOV R0, 0x0
		MSR PSP, R0
	}
	Open_Task_Scheduler();
}

//-----------------------------------------------------------�����ں����� ����------------------------------------------------
//-----------------------------------------------------------����Ĵ����ͼ��� ��ʼ------------------------------------------------
/*
* 1������TOR������SPC��
* 2������hash����
* 3������û����ĺϷ���
*/
int Task_Create(TASK_FUNCTION taskFunction, \
    void* Args, \
    unsigned char Priority, \
    unsigned char* icon, \
    char* Task_name, \
    StackDepth size) {
    //����һ��TORָ��;
    TASK_ORBIT_RING* node = NULL;
    //���봴������
    // �������ƹ淶���
    //�ַ�����TASK_NAME_SIZE+1λ��\0;
    //�������Ƴ���
    char task_name_temp[TASK_NAME_SIZE + 1];
    task_name_temp[TASK_NAME_SIZE] = '\0';
    if (strlen(Task_name) <= TASK_NAME_SIZE) {
        for (unsigned char i = 0; i < strlen(Task_name) + 1; i++) {
            task_name_temp[i] = Task_name[i];
        }
    }
    else {
        return -1;//��������
    }
    //�������Ƿ�����ظ�
    if (HashTable_Find(task_name_temp)) {
        return -4;//�������Ѿ�����
    }

    int value = TOR_Create(taskFunction, Args, Priority, icon, task_name_temp, size, &node);
    if (value) {//���񴴽�
        return value;
    }
    //����hash��
    if (HashTable_Insert(node->self_handle)) {
        //����ʧ��
    #if HEAP_SCHEME == HEAP_SOLUTION_1  
        hj_free(node, sizeof(TASK_ORBIT_RING));
    #endif

        return -1;
    }
    return 0;
}

//�ƻ����˺�������Ϊԭ�Ӻ������Է���ʼ���Ĺ����б����
int Task_Load(TASK_ORBIT_RING* TOR) {
    if (!TOR) {
        return -1;//�����쳣
    }
    //��������Ƿ��Ѿ�������
    if (TOR->Task_Enable_Flag == TOR_ENABLE_FLAG) {
        //��������Ѿ��������򷵻�
        return 1;
    }
flag:;//��������֮���ٴ���������������
    //����������
    if (TOR_Enable(TOR)) {
        //����1
        #if TASK_LOAD_SOLUTION == TASK_LOAD_SOLUTION_1 
            return -1;//�������ʧ��
        //����2
        #elif TASK_LOAD_SOLUTION == TASK_LOAD_SOLUTION_2

            SCHEDULING_PRIORITY_CHAIN* temp_spc = SPC_Head;
     
            //���ҵ�ǰ���л�Ծ����Ļ�
            do {
                temp_spc = temp_spc->next;
            } while (temp_spc != NULL && temp_spc->Ring_Active_Amount == 0);
            
           //������ѯ���
            //���1��������ִ��ڻ�Ծ�Ļ�����������������ȼ����ڴ˻����ܾ���������������
            if (temp_spc != NULL && TOR_Priority_Get(TOR) < temp_spc->Ring_Priority) {
                return -1;
            }
            //���2�����SPC�������ڻ�Ծ�Ļ������ߴ��ڻ�Ծ�Ļ���������������ȼ����ڻ���ڴ˻����������������
            //Ѱ��һ��������ȼ�����̬��TOR����
			//���ֲ��ԣ�������ڹ������ô����ɾ��������������û����ôɾ��һ������״̬������
			
            TASK_ORBIT_RING* temp_tor = TOR_Lowest_Enable_Designated_Status_Find(TASK_OBSTRUCT);
            if (!temp_tor) {
				temp_tor = TOR_Lowest_Enable_Designated_Status_Find(TASK_READY);
				if(!temp_tor)
					return -1;//SPC���������쳣
            }else{
				TASK_ORBIT_RING* temp_tor2 = TOR_Lowest_Enable_Designated_Status_Find(TASK_READY);
				if(temp_tor2 && TOR_Status_Get(&temp_tor->tactics) > TOR_Status_Get(&temp_tor2->tactics)){
					temp_tor = temp_tor2;
				}						
			}
			
			
            //��ֹ������
            Task_Exit_Designated(temp_tor);
            //��������������
            goto flag;
        
        //����3
        #elif TASK_LOAD_SOLUTION == TASK_LOAD_SOLUTION_3
            //��ȡSPC�����ȼ���͵��Ѿ������õĻ�
            TASK_ORBIT_RING* temp_tor = TOR_Lowest_Enable_Find();
            if (!temp_tor) {
                //���������õĻ�
                return -1;
        }
            //����Ƿ񷵻ص�TOR���ȼ���������������ȼ�
            if (temp_tor->self_SPC->Ring_Priority >= TOR_Priority_Get(TOR)) {
                return -1;//������ڻ��ߵ�����ܾ������������
            }
            //��ֹ������
            Task_Exit_Designated(temp_tor);
            //��������������
            goto flag;
        #endif
    }
    TOR->self_SPC->Ring_Active_Amount++;
    //��ʼ��ջ
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
    //��������Ϊ����̬
    TOR_Status_Set(&TOR->tactics, TASK_READY);
    TOR->Task_Enable_Flag = TOR_ENABLE_FLAG;//��������Ϊ����״̬
	TaskQueueUpdated = NEW_TASK_AVAILABLE;  //������������־ʹ��
    return 0;
}
//-----------------------------------------------------------����Ĵ����ͼ��� ����------------------------------------------------


//-----------------------------------------------------------��������ȼ����� ��ʼ------------------------------------------------
//�޸��������ȼ�
//�������Σ��ڷ������ȼ���ת��ǰ���£����������ȼ��ᵼ��֮ǰ��SPC�����٣�\
Ŀǰ�������ǣ����������ΪAPI��Ȼ�������һ����ʱ���ȼ��������������SPC��Ϊ��Ҳ��������ԭSPC
int Task_Priority_Modify(TASK_ORBIT_RING* TOR, unsigned char Priority) {
    if (!TOR) {
        return -1;//û�д�����
    }

    if (TOR->self_SPC->Ring_Priority == Priority) {
        return -1;//��Ҫ�޸ĵ����ȼ���ԭ���ȼ���ͬ������Ҫ�޸ģ��˳�
    }

    TOR_Priority_Set(TOR, Priority);//�޸����ȼ�

    SCHEDULING_PRIORITY_CHAIN* temp = TOR->self_SPC;

    if (TOR_Move(TOR)) {
        return -1;//���ȼ��޸�ʧ��
    }

    //�ݼ�ԭSPC�����������SPC��TOR�ڵ�Ϊ0�����������SPC
    if (--temp->Ring_Task_Amount == 0) {
        if (SPC_Delete(temp->Ring_Priority)) {
            return -1;//ɾ��ʧ��
        }
    }
    return 0;
}

//-----------------------------------------------------------��������ȼ����� ��ʼ------------------------------------------------

//-----------------------------------------------------------����Ĺ�������ֹ ��ʼ------------------------------------------------
/*
* �������
*/
void Task_Suspended(void) {
    //��ȡ��ǰ̽ͷ
    SCHEDULING_PRIORITY_CHAIN* SPC = Get_SPC_Detector();
	
	__disable_irq(); // �ر�ȫ���ж�
    //����ǰTOR��״̬����Ϊ����
    TOR_Status_Set(&SPC->Task_Ring->tactics, TASK_OBSTRUCT);
	
    /*����ϵͳ�δ�ʱ���������á�
    * ԭ�����£�
    * ��1�����ú��ܹ��ú������ж��г����ʱ��ִ�У��������һ�ּ��ȼ��˵���������·������ж�Ϊ���˲�䣬
    *���ǻ�û��ִ�д������������һ�̣��ں��л������񣬴˲������ᵼ���ж�ʧЧ���ο��·�ע�����1�����Ӷ�����ϵͳʱ��Ƭ�˷�
    * ��2��ʱ��Ƭ��������˷ѣ�����ͨ������ʱ��Ƭ�����Ǵ�����ִ�к���������������л�״̬
    */

    //�δ�ϵͳ��ʱ�����ô���...
    //Reset_Time_Slice_Timing();
	

    /*��������Ƿ����
    *���ô��жϵ�ԭ������:
    * ��һ������״̬������Ϊ�˹���̬�󣬱�����ֻ�Ǹı��˱�־λ����ǰ�����������У�������Ҫ�л����񳹵���ϵͳ��Ϊ������������̬��
    * ���Դ�ʱ�п��ܳ������������
    * ��1����������Ϊ����̬��������ٽ����ں����÷����������л��źţ���Ȼ����ɹ������Ǻ����������������л��Ĵ��뻹û��ִ�С�
    *����һ�δ������ٴμ�������Ծ̬��ʱ��˳��ִ�л�ִ�е������л��������ᵼ�����������л����Ӷ���������񱻴����������һ��ʱ��Ƭ��
    * ��2����������Ϊ����̬��ʱ��Ƭ���ܳ�ԣ����ô�������ͻ�ִ�������л�����������Ԥ����������������
    * �����������
    * ��1���ڴ˴�����һ���жϣ���Ϊ���������������1����ô�ٴν���ִ�и�����ʱ���ض����ڻ�Ծ̬�����Դ�ʱ��Ӧ����ִ�д���������
    * ��2������ʱ���жϷ���״̬Ϊ����ʱ���ڹ���״̬��ִ�д�����쳣���ֻ�ᷢ�����״ν���������ʱ�򣬴�ʱ���������û����ɣ�
    *����Ӧ������ִ�е�ǰ�������������񴥷����Ĵ���
    */
    //��ǰSPC�����Ծ���Լ�
    SPC->Ring_Active_Amount--;
	
	__enable_irq(); // ����ȫ���ж�
	
    if (TOR_Status_Get(&SPC->Task_Ring->tactics) == TASK_OBSTRUCT) {
        //���������������̼��������л������л�����
        User_Initiated_Switch_Trigger();
    }
}
//����ָ������
int Task_Suspended_Designated(TASK_ORBIT_RING* TOR) {
    if (!TOR) {
        return -1;//������δ��ע��
    }
    /*�˺��������������ĺ���������Ҫ���ӵķ��������߼�
    * �����������쳣���
    * ��1�����ж�����δ����״̬ʱ����ʱ��ֱ�ӷ��أ�����չ�����жϵ��ٽ�㣬
    *�ں��л���������������ָ������Ҳ��Ӱ�죬��Ϊ���жϵ���һ���Ѿ�ȷ��������״̬��֮�������״������ǰ������ҪΪ���Ǹ���
    * ��2�����жϹ���ʱ�����ж��Ƿ��ڻ�Ծ̬���ٽ�㣬�ں˾�����������л�������
    * ���Ŀ�������Ϊ�˹���̬����ô�ú����Ĳ�����Ӱ�졣
    * ���Ŀ����������Ϊ��Ծ̬����ô�ú����޸�Ŀ��״̬���������С�
    */
    //��������Ƿ�����
    if (TOR->Task_Enable_Flag == TOR_DISABLE_FLAG) {
        //�������û�б����ã��򷵻�
        return -1;
    }
    //���ù���
    if (TOR_Status_Get(&TOR->tactics) != TASK_OBSTRUCT) {
        TOR_Status_Set(&TOR->tactics, TASK_OBSTRUCT);
    }
    //Ŀ�껷��SPC�������Ծ������
    TOR->self_SPC->Ring_Active_Amount--;
    return 0;
}

/*
* �������
*/
//������ǰ����
int Task_Exit(void) {
    //��ȡ��ǰ̽ͷ
    SCHEDULING_PRIORITY_CHAIN*SPC = Get_SPC_Detector();

    //�ر�����
    if (TOR_Disable(SPC->Task_Ring)) {
        return -1;//��ջ�ͷ�ʧ��
    }

    /*������������л������������������
    * ��1�������������ȫ���жϴ򿪵�˲�䣬�ں˷����������л�����ʱ���ڸ������Ѿ������˹رգ���Դ�ͷţ�׼����
    *��������������CPUռ�к󳹵׽���ر�״̬��
    * ��2�������������ʱ��Ƭ��ԣ��ʱ�򣬴�ʱ����ִ�е��������������̽��������л����ڽ����CPUռ�к����ر�״̬
    */
    User_Initiated_Switch_Trigger();

    return 0;
}
//����ָ������
int Task_Exit_Designated(TASK_ORBIT_RING* TOR) {
    if (!TOR) {
        return -1;//������δ��ע��
    }
    /*
    * ��1��ͨ�������⣬��������û�����ã���ʱ�������أ��������жϷ��ص��ٽ��������Ŀ�����񣬵��ǵ�ǰ�������ɲ��ö��为��
    * ��2���ڼ�����������ж���ɺ͹ر������жϿ�ʼ���ٽ�㣬�����������ĳЩԭ��رգ�
    * 
    */
    //�ر�����
    if (TOR_Disable(TOR)) {
        return -1;//��ջ�ͷ�ʧ��
    }
    /*����ʹ�ò�������ԭ�����£�
    * ��һ��ʹ�ô˺������������Ҫ�Խϸߣ�������غ�����Ҫʹ�õ��ú�����
    *�ڼ��ز����У���Ҫɾ�������ȼ����������ظ����ȼ���������ȽϽ���������ڼ���״̬�£�����һ���ں˵������л������Σ���������Ը����������
    * �ڶ���Ϊ�˾������ø����ȼ�����ռ����Դ�����������ȼ�����ͬ���ȼ������Ƴ�һ��ʱ��ƬҲ��������ġ�
    */
    return 0;
}
//-----------------------------------------------------------����Ĺ�������ֹ ����------------------------------------------------


//-----------------------------------------------------------������ ��ʼ------------------------------------------------
//������
int Task_Wake(TASK_ORBIT_RING* TOR) {
    if (!TOR) {
        return -1;//������δ��ע��
    }
    //��������Ƿ�����
    if (TOR->Task_Enable_Flag == TOR_DISABLE_FLAG) {
        //�������û�б����ã��򷵻�
        return 0;
    }
	__disable_irq(); // �ر�ȫ���ж�
	//�������״̬
	if(TOR_Status_Get(&TOR->tactics) == TASK_OBSTRUCT){
	//ֻ�е������ڹ����ʱ����ܻ���
		TOR_Status_Set(&TOR->tactics, TASK_READY);
		//������SPC���Ļ�Ծ��������
		TOR->self_SPC->Ring_Active_Amount++;
		//�������ѣ����������־λʹ��
		TaskQueueUpdated = NEW_TASK_AVAILABLE;
	}
	__enable_irq(); // ����ȫ���ж�
    return 0;
}
//-----------------------------------------------------------������ ����------------------------------------------------


//-----------------------------------------------------------���������ַ�Ĳ��� ��ʼ------------------------------------------------

/*
* �������
*/

TASK_ORBIT_RING* Task_Find(char* Task_Name) {
    return HashTable_Find(Task_Name);
}
//-----------------------------------------------------------���������ַ�Ĳ��� ����------------------------------------------------

//��ȡ��ǰ����ľ��
TASK_ORBIT_RING* Task_Current_Handle(void){
	SCHEDULING_PRIORITY_CHAIN* spc = Get_SPC_Detector();
	return spc->Task_Ring;
}







//�������ȼ����񴴽�(������������������ȼ�֮�⣬�����ȼ�������ִ��)
#if SPECIAL_PRIORITY_CONFIG	== SPECIAL_PRIORITY_ENABLE
int Special_Priority_Task_Create(\
	TASK_FUNCTION taskFunction,\
	void* Args,\
	unsigned char* icon,\
	char* Task_name,\
	StackDepth size){
	//���������
	char task_name_temp[TASK_NAME_SIZE + 1];
    task_name_temp[TASK_NAME_SIZE] = '\0';
    if (strlen(Task_name) <= TASK_NAME_SIZE) {
        for (unsigned char i = 0; i < strlen(Task_name) + 1; i++) {
            task_name_temp[i] = Task_name[i];
        }
    }
    else {
        return -1;//��������
    }
    //�������Ƿ�����ظ�
    if (HashTable_Find(task_name_temp)) {
        return -4;//�������Ѿ�����
    }
	#if HEAP_SCHEME == HEAP_SOLUTION_1  	
	TASK_ORBIT_RING* TOR = (TASK_ORBIT_RING*)hj_malloc(sizeof(TASK_ORBIT_RING));
	#endif
	if(!TOR){
		return -1;//�ռ����ʧ��
	}
	
	//����TOR
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
	
	//����hash��
	if(HashTable_Insert(TOR->self_handle)){	
        //����ʧ��
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



//��������ʱ�ṹ��
/*
* ��1��ֻ������ʽ�����Ż���뻥�����ȴ�����
* ��2�������л�ʱ���Ὣ���ж�ʱ��������ʱ���Լ�
* ��3����ʱ�ṹ�尴�����ȼ����С�
*/


/*************************
* ����ͬ��
*************************/
//-------------------------------------------------------����ͬ�� ��ʼ--------------------------------

//������ȴ����������һ���ڵ�
Task_Wait_Tracker* TWT_Add(TASK_ORBIT_RING* TOR, unsigned int systick_time, void* Synchronization_Primitives, unsigned char block_type) {
	#if HEAP_SCHEME == HEAP_SOLUTION_1  //��������1
	Task_Wait_Tracker* node = (Task_Wait_Tracker*)Ink_malloc(sizeof(Task_Wait_Tracker));//����ռ�
	#endif
	if (!node) {
		//�ռ�����ʧ��
		return NULL;
	}
	//��ʼ����
	node->TOR = TOR;
	node->systick_time = systick_time;
	//node->mutex = mutex;
	node->Synchronization_Primitives = Synchronization_Primitives;
	node->block_type = block_type;
	node->enable_flag = 1;
	node->next = NULL;

	//����
	if (!TWT_Header) {
		//���TWTû�нڵ�
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

//ɾ��ָ������ȴ��ڵ�
void TWT_Delete(Task_Wait_Tracker* TWT_Anchor_Point) {
	if (!TWT_Header) {
		//TWT����Ϊ��
		return;
	}

	Task_Wait_Tracker* fast = TWT_Header;
	Task_Wait_Tracker* slow = NULL;

	if (TWT_Anchor_Point == TWT_Header) {
		//���Ҫɾ���Ľڵ���ͷ���
		TWT_Header = TWT_Header->next;
	}
	else {
		//���ɾ���Ľڵ㲻��ͷ���
		while (fast != NULL) {
			if (fast == TWT_Anchor_Point) {
				break;
			}
			slow = fast;
			fast = fast->next;
		}

		slow->next = fast->next;//�޳�Ŀ��TWT
	}
#if HEAP_SCHEME == HEAP_SOLUTION_1  //��������1
	Ink_free(fast, sizeof(Task_Wait_Tracker));//�ͷŶ����ռ�
#endif
	return;
}



//��ʼ����ͨ��������ע�⣬�ú���ֻ����main��ʹ�ã�
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



//��ͨ����������(������)
int Mutex_Lock_Non_Blocking(Mutex_t* Mutex) {
	if (Mutex->Initialization_flag) {
		return -2;//���������û�г�ʼ��
	}

	//��ȡ��ǰSPC
	SCHEDULING_PRIORITY_CHAIN* SPC = Get_SPC_Detector();


	//������״�����
	/*�����������
		* ��1�������ʱ��ͨ���˵�ǰif�жϣ����ǻ�û����������Ȩ��ʱ�������л���
		* ����һ���������������������Ȩ���������ô�������л�������ʱ�����������Ȩ�ͻᱻ��ǰ���ָ��ǵ����
	*/
	//�ر�ȫ���жϣ���֤ԭ����
	//�ر�ȫ���жϴ���...
	__disable_irq();
	if (Mutex->Possessor == NULL) {
		Mutex->Possessor = SPC->Task_Ring;//������������Ȩ
		Mutex->Status = 1;//����Ϊ����״̬
		
		//����ȫ���жϴ���...
		__enable_irq();
		return 0;
	}

	//�ж��Ƿ�����
	if (Mutex->Status == 0) {
		//���û������
		Mutex->Possessor = SPC->Task_Ring;//������������Ȩ
		Mutex->Status = 1;//����Ϊ����״̬
		//�����ɹ�
		//����ȫ���жϴ���...
		__enable_irq();
		return 0;
	}
	//����ȫ���жϴ���...
	__enable_irq();
	//����Ѿ���������-1
	return -1;
	
}


//��ͨ����������(����),��ʱ��ƬΪ��λ��1ʱ��Ƭ���ȡ�2ʱ��Ƭ���ȣ������Ϊ0��������������ֱ����������Դ�ͷ�
int Mutex_Lock(Mutex_t* Mutex,unsigned int Block_Time) {
	//��⻥�����Ƿ��ʼ��
	if (Mutex->Initialization_flag) {
		return -2;//���������û�г�ʼ��
	}

	//��ȡ��ǰSPC
	SCHEDULING_PRIORITY_CHAIN* SPC = Get_SPC_Detector();

	//�����ͬһ��TOR����
	if (Mutex->Status == 1 && Mutex->Possessor == SPC->Task_Ring) {
		/*
		* ���ڻ������Ѿ�����ǰ����������ֱ�ӷ��ء�
		*���ﲻ��Ҫ�ر�ȫ���жϣ���Ϊ�������ǰ����������
		*��ô�����������޷������ģ����Բ�������̲߳���ȫ
		*/
		return 0; 
	}


	/*���������ụ�����Ĺ��̿��������̲߳���ȫ�����Դ�����Ҫ�����ٽ�����Ҳ���ǹر�ȫ���ж�
	* 
	* �����������
	* ��1�������ʱ��ͨ���˵�ǰif�жϣ����ǻ�û����������Ȩ��ʱ�������л���
	* ����һ���������������������Ȩ���������ô�������л�������ʱ�����������Ȩ�ͻᱻ��ǰ���ָ��ǵ����
	*/

	//���������1���״����ụ������
	__disable_irq();
	//�ж��Ƿ��״�����
	if (Mutex->Possessor == NULL) {
		/*ֻ�е�����ȨΪNULL��ʱ�����״�����.
		* ��Ϊ�������������������ͷŵ�ʱ��ֻ������״̬���ģ�����Ȩ��ֵû�иı䣬���ǵȴ���ֵ����
		*/
		Mutex->Possessor = SPC->Task_Ring;//������������Ȩ
		Mutex->Status = 1;//����Ϊ����״̬
		//�ָ�ȫ���ж�
		__enable_irq();
		return 0;
	}
	//����ȫ���жϣ��״������жϺ���״��ж���������������м�����������
	__enable_irq();
 
	/*���������2�����״����ụ������
	*���״����ụ�����������������ͼ�ʱ�ڵĶ�ʱ������һֱ�����ụ��������ѭ��
	*/

	//�������ụ�����ı�Ҫ����
	unsigned char Task_Wait_Flag = 0;//�Ƿ�����˵ȴ����б�־, 0Ϊû�м��룬��0Ϊ�Ѿ�����
	Task_Wait_Tracker* TWT_Anchor_Point = NULL;//���ָ�����ڶ�λ��ǰ�����ڵȴ������е�λ��

FLAG:;//��������������ת��ǩ
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;//�ر�������ȣ���ֹ����������𣬿����жϵ��ٽ�����������ȣ���������������������
	//�ر�ȫ���жϣ�ԭ�Ӳ������������
	__disable_irq();
	//���������û�б�����
	if (Mutex->Status == 0) {
		//�������ụ����������Ȩ
		Mutex->Possessor = SPC->Task_Ring;//������������Ȩ
		Mutex->Status = 1;//����Ϊ����״̬
		if (Task_Wait_Flag) {
			//�����ǰ������ӹ�TWT,��TWT�����Ƴ��ڵ�,�ɹ����ụ�����󣬲����Ƕ�ʱ�������û�������������Ҫ��ʹ��TWT�ڵ㣬����Ӧ��ɾ��
			TWT_Delete(TWT_Anchor_Point);
		}
		SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;//�����������
		//����ȫ���ж�
		__enable_irq();
		return 0;
	}

	//����������Ѿ���������ô���������״̬��������Ϣ��������ȴ�������
	//���ݶ�ʱ������������������
	if (!Task_Wait_Flag) {//�ж��Ƿ��Ѿ������˵ȴ����У����û�м�����������롣
		if (Block_Time == 0) {
			//��������
			
			TWT_Anchor_Point = TWT_Add(SPC->Task_Ring, 0, Mutex, 0);
		}
		else {
			//��ʱ����
			TWT_Anchor_Point = TWT_Add(SPC->Task_Ring, Block_Time, Mutex, 1);
		}

		//����Ƿ����TWT���ɹ�
		if (!TWT_Anchor_Point) {
			//����ȫ��
			__enable_irq();
			return -1;//���м���ʧ��
		}

		Task_Wait_Flag = 1;//���µȴ�������ӱ�־
	}

	//����ϵͳ�δ��ʱ����Ԥ�����·����뿪���жϵ�˲�䣬�����ڱ�����֮���ȱ����ѣ���������������Ӷ���������
	SysTick->VAL = 0;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	//����ȫ��
	__enable_irq();
	//����ǰ����
	Task_Suspended();

	//�жϱ����ѵķ�ʽ
	//��ʽ1����ʱʱ�䵽
	if (TWT_Anchor_Point->block_type == 1 && TWT_Anchor_Point->systick_time == 0) {
		//�ж�����������Ϊ��ʱ����          //�ж϶�ʱʱ��Ϊ0

		//�Ӷ������Ƴ�TWT
		TWT_Delete(TWT_Anchor_Point);
		return 0;
	}
	//��ʽ2���������ͷţ�����һ�������߻���

	//�����񱻻��ѵ�ʱ�������������ụ��������Ȩ
	goto FLAG;
}

 
//��ͨ����������
int Mutex_Unlock(Mutex_t* Mutex) {

	/*���������1��
	* ��֤��ǰ�������Ƿ���й���ʼ��
	*/
	if (Mutex->Initialization_flag) {
		return -2;//δ��ʼ��
	}

	//��ȡ��ǰSPC
	SCHEDULING_PRIORITY_CHAIN* SPC = Get_SPC_Detector();

	/*���������2��
	* ���Ŀ�껥��������δ����״̬�������̷���
	*/

	//�ر�ȫ���ж�
	__disable_irq();
	//��֤�Ƿ���������1���������״̬����2���������Ȩ�Ƿ�ΪNULL
	if (!Mutex->Status || Mutex->Possessor == NULL) {
		//û���������߳�ʼ��֮��û�����������
		//����ȫ���ж�
		__enable_irq();
		return 0;
	}


	/*���������3��
	* �����ǰ�������Ѿ���������������֤���������Ƿ�������������
	*/

	//��֤����������
	if (Mutex->Possessor != SPC->Task_Ring) {
		//����Ȩ��֤ʧ��
		//����ȫ���ж�
		__enable_irq();
		return -1;
	}

	//���������������������
	//�������״̬
	Mutex->Status = 0;
	//����һ���ȴ��Ľڵ�
	
	Task_Wait_Tracker* TWT_Anchor_Point = TWT_Header;

	while (TWT_Anchor_Point != NULL) {
		//��֤����ҵ�һ���ڵ㴦��ʹ��״̬�������䱣��Ļ�������ַ���ڵ�ǰ�Ļ�������ַ����ͬһ���������������Ѹ�����
		if (TWT_Anchor_Point->enable_flag == 1 && TWT_Anchor_Point->Synchronization_Primitives == Mutex) {
			//���Ѹ�����
			Task_Wake(TWT_Anchor_Point->TOR);
			////ʧ������ڵ��״̬
			//TWT_Anchor_Point->enable_flag = 0;
			//����ȫ���ж�
			__enable_irq();
			return 0;
		}
		//����ê��
		TWT_Anchor_Point = TWT_Anchor_Point->next;
	}

	//����ȫ���ж�
	__enable_irq();
	return 0;
}




/*���ú���ʵ�ֵĹ������¡�
* ��1���ú���Ӧ���������л����ٽ���ʹ��
* ��2���ú�����Ե��������໥�����������������Ͷ�ʱ����
* ��3��ÿһ�ε��øú�����Ӧ������ʱ������Ľڵ�systick_time�Լ�
* ��4��Ϊ�����Ч�ʣ�Ӧ�����Լ����жϣ������ʱ������systick_time��Ϊ0ʱ��Ӧ�����Ѷ�Ӧ�����񣬣�Ŀǰ���뷨�ǻ��ѵ�ͬʱҲɾ���ýڵ㣩
* ��5��������ڵ�Ķ�ʱ����֮�����̽��ڵ����ݽṹ�е�enbale_flag��־����Ϊʧ�ܡ�Ŀǰ���뷨���ýڵ�������������ɾ����
*/

//����ͬ��ԭ�Ŀǰû�з���bug��
void Synchronization_Primitives_Update(void) {
	//�������ȴ��ڵ��Ƿ�Ϊ��
	if (!TWT_Header) {
		//���Ϊ�գ������̷���
		return;
	}
	//����һ��ê�㣬���ڶ�TWT�����б���
	Task_Wait_Tracker* TWT_Anchor_Point = TWT_Header->next;

	//������������ǰû�б�����TWT����ĩβ
	while (TWT_Anchor_Point != NULL) {

		//�жϵ�ǰê�������Ƕ�ʱ����������������,ͬʱ�жϵ�ǰ��ê���Ƿ�����
		if (TWT_Anchor_Point->block_type == 0 || TWT_Anchor_Point->enable_flag == 0) {
			//���Ϊ������������û�б�����,����ê�㲢���½���ѭ��������
			TWT_Anchor_Point = TWT_Anchor_Point->next;
			continue;
		}

		//����Ƕ�ʱ������
		//���Լ�һ��ʱ�䵥λ����Ϊ���е�ʱ������Ǵ���0�ģ��������������Ϸ�������ˣ����Բ�������쳣���
		if (--TWT_Anchor_Point->systick_time == 0) {
			//��ʱ��Ϊ0��ʱ������Ҫ���̻���ָ��������
			//����ǰ�ڵ�����Ϊʧ�ܣ���ֹ��û�б�ɾ���������±�������ã�ע�⣬�ڵ������ɵ�ǰ�����ͷ��ڴ棬���������������ڽ��յ�ʧ����Ϣ��ɾ����
			TWT_Anchor_Point->enable_flag = 0;
			//��������
			Task_Wake(TWT_Anchor_Point->TOR);
		}

		//���ʱ�䲻Ϊ0����ô���½���ѭ��
		TWT_Anchor_Point = TWT_Anchor_Point->next;
	}
	return;
}

//--------------------------------------------------------------------------����������------------------------------------

//��ʼ���ź���
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
//�Բ������ķ�ʽ����һ���ź���
int  Semaphore_Applay_Non_Blocking(Semaphore_t* Semaphore) {
	if (Semaphore->Initialization_flag) {
		return -2;//���������û�г�ʼ��
	}

	//��ȡ��ǰSPC
	SCHEDULING_PRIORITY_CHAIN* SPC = Get_SPC_Detector();

	//������״�����
	/*�����������
		* ��1�������ʱ��ͨ���˵�ǰif�жϣ����ǻ�û����������Ȩ��ʱ�������л���
		* ����һ���������������������Ȩ���������ô�������л�������ʱ�����������Ȩ�ͻᱻ��ǰ���ָ��ǵ����
	*/
	//�ر�ȫ���жϣ���֤ԭ����
	//�ر�ȫ���ж�
	__disable_irq();
	//�����Դ��ʣ������
	if (Semaphore->Current_Count > 0) {
		Semaphore->Current_Count--;
		//����ȫ���ж�
		__enable_irq();
		return 0;
	}
	//����ȫ���ж�
	__enable_irq();
	//����ʧ�ܣ������˳�

	//����Ѿ���������-1
	return -1;

}

//���������루�����汾��
int Semaphore_Applay(Semaphore_t* Semaphore, int Block_Time) {
	if (Semaphore->Initialization_flag) {
		return -2;//�ź���û�г�ʼ��
	}

	//��ȡ��ǰSPC
	SCHEDULING_PRIORITY_CHAIN* SPC = Get_SPC_Detector();
	//�������ụ�����ı�Ҫ����
	unsigned char Task_Wait_Flag = 0;//�Ƿ�����˵ȴ����б�־, 0Ϊû�м��룬��0Ϊ�Ѿ�����
	Task_Wait_Tracker* TWT_Anchor_Point = NULL;//���ָ�����ڶ�λ��ǰ�����ڵȴ������е�λ��

FLAG:;//�����ź�������ת��ǩ
	/*
	* ���������1������������0�����ɱ�������
	*/
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;//�ر�������ȣ���ֹ����������𣬿����жϵ��ٽ�����������ȣ���������������������
	//�ر�ȫ���ж�
	__disable_irq();
	if (Semaphore->Current_Count > 0) {
		Semaphore->Current_Count--;
		if (Task_Wait_Flag) {
			//��������TWT����ô��TWT���Ƴ��ڵ�
			TWT_Delete(TWT_Anchor_Point);
		}
		//����ȫ��
		__enable_irq();
		return 0;
	}
	
	
	/*
	* ���������1������������0��������Դ�Ѿ������룬���񽫻�����ȴ���
	*/
	//�ж��Ƿ�����˵ȴ�����
	if (!Task_Wait_Flag) {
		//���û�м���
		if (Block_Time == 0) {
			//��������
			TWT_Anchor_Point = TWT_Add(SPC->Task_Ring, 0, Semaphore, 0);
		}
		else {
			//��ʱ����
			TWT_Anchor_Point = TWT_Add(SPC->Task_Ring, Block_Time, Semaphore, 1);
		}
			
		//����Ƿ����TWT���ɹ�
		if (!TWT_Anchor_Point) {
			//����ȫ��
			__enable_irq();
			return -1;//���м���ʧ��
		}
			
		Task_Wait_Flag = 1;//���µȴ�������ӱ�־
	}
	//����Ѿ�����
	//����ϵͳ�δ��ʱ����Ԥ�����·����뿪���жϵ�˲�䣬�����ڱ�����֮���ȱ����ѣ���������������Ӷ���������
	SysTick->VAL = 0;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	//����ȫ��
	__enable_irq();
	//����ǰ����
	Task_Suspended();

	//�жϱ����ѵķ�ʽ
	//��ʽ1����ʱʱ�䵽
	if (TWT_Anchor_Point->block_type == 1 && TWT_Anchor_Point->systick_time == 0) {
		//�ж�����������Ϊ��ʱ����          //�ж϶�ʱʱ��Ϊ0

		//�Ӷ������Ƴ�TWT
		TWT_Delete(TWT_Anchor_Point);
		return 0;
	}
	//��ʽ2���ź�����Դ�ָ�����

	//�����񱻻��ѵ�ʱ�������������ụ��������Ȩ
	goto FLAG;

}

//�������黹
int Semaphore_Give(Semaphore_t* Semaphore) {
	//����Ƿ��ʼ��
	if (Semaphore->Initialization_flag) {
		//���û�г�ʼ�����򷵻ش���
		return -2;
	}

	//��������Դ�Ƿ�����
	if (Semaphore->Current_Count == Semaphore->Max_Count) {
		//���������Դ�����ģ���ô�������ͷ�
		return 0;
	}

	/*��������ռ�ÿ�����Դʱ
	* �����1�����ֿ�����Դ��ռ�ã����ǻ��в�����Դʣ�ࡿ
	* �����2�����еĿ�����Դ����ռ�á�
	*/

	//�ر�ȫ���ж�
	__disable_irq();

	//���1
	if (Semaphore->Current_Count > 0) {
		//��Դ��ʣ�࣬����Ҫ���ǵ��������Դ���������Ϊ�Ѿ���������
		//������Դ��ʣ�࣬����Ҳ�����������ڵȴ����������������ֻ��Ҫ�ͷ��ź�������
		Semaphore->Current_Count++;
		//����ȫ���ж�
		__enable_irq();
		return 0;
	}

	//���2����Ҫ���ȴ������е�һ�������� 

	Task_Wait_Tracker* TWT_Anchor_Point = TWT_Header;

	while (TWT_Anchor_Point != NULL) {
		//��֤����ҵ�һ���ڵ㴦��ʹ��״̬�������䱣��Ļ�������ַ���ڵ�ǰ�Ļ�������ַ����ͬһ���������������Ѹ�����
		if (TWT_Anchor_Point->enable_flag == 1 && TWT_Anchor_Point->Synchronization_Primitives == Semaphore) {
			//���Ѹ�����
			Task_Wake(TWT_Anchor_Point->TOR);
			////ʧ������ڵ��״̬
			//TWT_Anchor_Point->enable_flag = 0;
			Semaphore->Current_Count++;
			//����ȫ���ж�
			__enable_irq();
			return 0;
		}
		//����ê��
		TWT_Anchor_Point = TWT_Anchor_Point->next;
	}
	Semaphore->Current_Count++;
	//����ȫ���ж�
	__enable_irq();
	return 0;
}


//----------------------------------------------����ͬ�� ����-----------------------------------------
