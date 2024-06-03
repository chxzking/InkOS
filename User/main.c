#include "OLED.h"
#include "inkos.h"


//示例：
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
	OLED_Init();//屏幕初始化
	Inkos_Init();//操作系统初始化
	
	
	//创建两个任务
	Task_Create(Task1,NULL,8,NULL,"Task1",64);
	Task_Create(Task2,NULL,8,NULL,"Task2",64);
	
	//启动任务
	Task_Load(Task_Find("Task1"));
	Task_Load(Task_Find("Task2"));
	
	Inkos_StartUp();//操作系统启用
	while(1);
		
}
