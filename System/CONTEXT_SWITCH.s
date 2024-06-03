
	
NVIC_INT_CTRL	EQU		0xE000ED04    	;�жϿ��ƺ�״̬�Ĵ����ĵ�ַ
NVIC_SYSPRI14	EQU		0xE000ED22		;ϵͳ���������ȼ��Ĵ���14�ĵ�ַ
NVIC_PENDSV_PRI	EQU			  0xFF		;PendSV�쳣�����ȼ�ֵ
NVIC_PENDSVSET	EQU		0x10000000		;PendSV�쳣����ֵ
	
	PRESERVE8
	THUMB
	
	AREA    MYCODE, CODE, READONLY	
		
	EXTERN SPC_Detector
	EXTERN Task_Switch		
	
PendSV_Handler PROC	
	EXPORT PendSV_Handler
		
; �ر��ж�
    CPSID I


; ��� PSP, ������ǵ�һ�����񣬷�ֹ����δ������ΪӦ����������
    MRS R0, PSP
	CBZ R0, Load_New_Task

Save_Current_Task;��������
    MRS R0, PSP
    STMDB R0!, {R4-R11}
    LDR R1, =SPC_Detector ;��ȡ��̽ͷ�ĵ�ַ
    LDR R1, [R1];�����ó�̽ͷ�е�SPCֵ
	LDR R1, [R1];�����ó�̽ͷ�е�SPCֵ�µ�TOR��ַ
    STR R0, [R1]

Select_Next_Task;ѡ����һ������
	PUSH {LR}
    BL Task_Switch
	POP {LR}

Load_New_Task;��������
    LDR R0, =SPC_Detector
    LDR R0, [R0]
	LDR R0, [R0]
    LDR R0, [R0]
    LDMIA R0!, {R4-R11}
    MSR PSP, R0
	ORR LR, LR, #0x4 ; R1 |= 0x04 : lr |= 32'b0000_0000_0000_0100

; ���ж�
    CPSIE I

; return
    BX LR

	ENDP
		
	NOP
	
	END
	
	
	
	
	



;NVIC_INT_CTRL	EQU		0xE000ED04    	;�жϿ��ƺ�״̬�Ĵ����ĵ�ַ
;NVIC_SYSPRI14	EQU		0xE000ED22		;ϵͳ���������ȼ��Ĵ���14�ĵ�ַ
;NVIC_PENDSV_PRI	EQU			  0xFF		;PendSV�쳣�����ȼ�ֵ
;NVIC_PENDSVSET	EQU		0x10000000		;PendSV�쳣����ֵ
	
	;PRESERVE8
	;THUMB
	
	;AREA    MYCODE, CODE, READONLY	
		
	;EXTERN SPC_Detector
	;EXTERN Task_Switch		
	
;PendSV_Handler PROC	
	;EXPORT PendSV_Handler
		
;; �ر��ж�
    ;CPSID I



    ;MRS R0, PSP
	;CBZ R0, first_switch


;save_context;��������
    ;MRS R0, PSP
    ;STMDB R0!, {R4-R11}
    ;LDR R1, =SPC_Detector ;��ȡ��̽ͷ�ĵ�ַ
    ;LDR R1, [R1];�����ó�̽ͷ�е�SPCֵ
	;LDR R1, [R1];�����ó�̽ͷ�е�SPCֵ�µ�TOR��ַ
    ;STR R0, [R1]

;select_next_TCB;ѡ����һ������
	;PUSH {LR}
    ;BL Task_Switch
	;POP {LR}
	;B restore_context

;first_switch
	;BL Task_Switch
	;B restore_context

;restore_context;��������
    ;LDR R0, =SPC_Detector
    ;LDR R0, [R0]
	;LDR R0, [R0]
    ;LDR R0, [R0]
    ;LDMIA R0!, {R4-R11}
    ;MSR PSP, R0
	;ORR LR, LR, #0x4 ; R1 |= 0x04 : lr |= 32'b0000_0000_0000_0100

;; ���ж�
    ;CPSIE I

;; return
    ;BX LR

	;ENDP
		
	;NOP
	
	;END


