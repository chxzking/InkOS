
	
NVIC_INT_CTRL	EQU		0xE000ED04    	;中断控制和状态寄存器的地址
NVIC_SYSPRI14	EQU		0xE000ED22		;系统处理器优先级寄存器14的地址
NVIC_PENDSV_PRI	EQU			  0xFF		;PendSV异常的优先级值
NVIC_PENDSVSET	EQU		0x10000000		;PendSV异常触发值
	
	PRESERVE8
	THUMB
	
	AREA    MYCODE, CODE, READONLY	
		
	EXTERN SPC_Detector
	EXTERN Task_Switch		
	
PendSV_Handler PROC	
	EXPORT PendSV_Handler
		
; 关闭中断
    CPSID I


; 检查 PSP, 如果这是第一个任务，防止发生未定义行为应该跳出保护
    MRS R0, PSP
	CBZ R0, Load_New_Task

Save_Current_Task;保存任务
    MRS R0, PSP
    STMDB R0!, {R4-R11}
    LDR R1, =SPC_Detector ;获取到探头的地址
    LDR R1, [R1];解引用出探头中的SPC值
	LDR R1, [R1];解引用出探头中的SPC值下的TOR地址
    STR R0, [R1]

Select_Next_Task;选择下一个任务
	PUSH {LR}
    BL Task_Switch
	POP {LR}

Load_New_Task;加载任务
    LDR R0, =SPC_Detector
    LDR R0, [R0]
	LDR R0, [R0]
    LDR R0, [R0]
    LDMIA R0!, {R4-R11}
    MSR PSP, R0
	ORR LR, LR, #0x4 ; R1 |= 0x04 : lr |= 32'b0000_0000_0000_0100

; 打开中断
    CPSIE I

; return
    BX LR

	ENDP
		
	NOP
	
	END
	
	
	
	
	



;NVIC_INT_CTRL	EQU		0xE000ED04    	;中断控制和状态寄存器的地址
;NVIC_SYSPRI14	EQU		0xE000ED22		;系统处理器优先级寄存器14的地址
;NVIC_PENDSV_PRI	EQU			  0xFF		;PendSV异常的优先级值
;NVIC_PENDSVSET	EQU		0x10000000		;PendSV异常触发值
	
	;PRESERVE8
	;THUMB
	
	;AREA    MYCODE, CODE, READONLY	
		
	;EXTERN SPC_Detector
	;EXTERN Task_Switch		
	
;PendSV_Handler PROC	
	;EXPORT PendSV_Handler
		
;; 关闭中断
    ;CPSID I



    ;MRS R0, PSP
	;CBZ R0, first_switch


;save_context;保存任务
    ;MRS R0, PSP
    ;STMDB R0!, {R4-R11}
    ;LDR R1, =SPC_Detector ;获取到探头的地址
    ;LDR R1, [R1];解引用出探头中的SPC值
	;LDR R1, [R1];解引用出探头中的SPC值下的TOR地址
    ;STR R0, [R1]

;select_next_TCB;选择下一个任务
	;PUSH {LR}
    ;BL Task_Switch
	;POP {LR}
	;B restore_context

;first_switch
	;BL Task_Switch
	;B restore_context

;restore_context;加载任务
    ;LDR R0, =SPC_Detector
    ;LDR R0, [R0]
	;LDR R0, [R0]
    ;LDR R0, [R0]
    ;LDMIA R0!, {R4-R11}
    ;MSR PSP, R0
	;ORR LR, LR, #0x4 ; R1 |= 0x04 : lr |= 32'b0000_0000_0000_0100

;; 打开中断
    ;CPSIE I

;; return
    ;BX LR

	;ENDP
		
	;NOP
	
	;END


