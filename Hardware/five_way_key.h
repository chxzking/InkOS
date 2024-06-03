#ifndef _FIVE_WAY_KEY
#define _FIVE_WAY_KEY

typedef enum KEY_DIRECTION KEY_DIRECTION ;

extern KEY_DIRECTION KeyNum;

enum KEY_DIRECTION{
	KEY_EMPTY,	//��������
	KEY_RESET,
	KEY_SET,
	KEY_MID,	//�м��
	KEY_RIGHT,	//��
	KEY_LEFT,	//��
	KEY_DOWN,	//��
	KEY_UP		//��
};

void FiveWayKey_Init(void);
uint8_t FiveKey_Num(void);

//char* FiveKey_char(void);



#endif
