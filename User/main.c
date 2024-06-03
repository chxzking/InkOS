#include "OLED.h"
#include "inkos.h"


//ʾ����
void Task1(void* args){
	Hj_String(0,0,"Task1:");
	int i = 0;
	while(1){
		Hj_Int(0,63,i++);
		print();
	}
}

void Task2(void* args){
	Hj_String(16,0,"Task2:");
	int i = 0;
	while(1){
		Hj_Int(16,63,i++);
		print();
	}
}

int main(){
	OLED_Init();//��Ļ��ʼ��
	Inkos_Init();//����ϵͳ��ʼ��
	
	
	//������������
	Task_Create(Task1,NULL,8,NULL,"Task1",64);
	Task_Create(Task2,NULL,8,NULL,"Task2",64);
	
	//��������
	Task_Load(Task_Find("Task1"));
	Task_Load(Task_Find("Task2"));
	
	Inkos_StartUp();//����ϵͳ����
	while(1);
		
}
