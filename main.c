/*
 * Default main.c for rtos lab.
 * @author Andrew Morton, 2018
 * stephen's branch
 * first commit
 */
#include <LPC17xx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define STACK_SIZE	1024

typedef void (*rtosTaskFunc_t)(void *args);

typedef enum{
	READY,
	RUNNING,
	BLOCKED,
	INACTIVE,
	TERMINATED
} state_t;

typedef enum{
	IDLE,
	LOW,
	NORMAL,
	ABOVE_NORMAL,
	HIGH
}priority_t;

typedef struct TCB{
	uint8_t task_id;
	state_t state;
	uint32_t stack_addr;
	priority_t priority;
} TCB_t;

int num_tasks = 0;
TCB_t TASKS[6];

TCB_t *currentTask, *readyTask;

volatile uint32_t msTicks = 0;
uint32_t stackPointer_current, stackPointer_next;

void Delay(uint32_t dlyTicks)
{
	uint32_t curTicks;
	curTicks = msTicks;
	
	while((msTicks - curTicks) < dlyTicks);
	
}

bool osKernelInitialize (void){
	
	uint32_t *vectorTable = 0x0;
	uint32_t mainStack = vectorTable[0];                     // = 0xFFFFF800 assuming main stack goes to 0xFFFFFFFF
	// initialize each TCB with the base address for its stack
	
	for(int i = 5; i>-1; i--)
	{
		// i = 5,4,3,2,1,0
		
		// TASKS[i].stack_addr =  mainStack - (STACK_SIZE/8)*(6-i);
		TASKS[i].stack_addr =  (mainStack - 4) - (STACK_SIZE/8)*(5-i);
		TASKS[i].task_id = i;
		TASKS[i].state = INACTIVE;
		TASKS[i].priority = IDLE;
	}
	
	return true;
}

void osThreadStart(rtosTaskFunc_t task, void *arg, priority_t priority)
{
	/*
	TCB_t *current_task = &TASKS[num_tasks++];
	
	current_task -> priority = priority;
	
	uint32_t *PSR = (uint32_t *)(current_task -> stack_addr + 15*sizeof(uint32_t));
	uint32_t *R0 = (uint32_t *)(current_task -> stack_addr + 8*sizeof(uint32_t));
	uint32_t *PC = (uint32_t *)(current_task -> stack_addr + 14*sizeof(uint32_t));
	
	*PSR = 0x01000000;
	*PC = (uint32_t)(task);
	*R0 = (uint32_t)(arg);
	
	// populate R4-R11
	for (int i = 0; i<8; i++)
	{
		uint32_t *curr = (uint32_t *)(current_task ->stack_addr + i*sizeof(uint32_t));
		
		*curr = 0x00000000 + (i+1);
	}
	
	for (int i = 9; i<14; i++)
	{
		uint32_t *curr = (uint32_t *)(current_task -> stack_addr + i*sizeof(uint32_t));
		
		*curr = 0xFFFF0000 + i;
	}
	*/
	
		TCB_t *current_task = &TASKS[num_tasks++];
	
	current_task -> priority = priority;
	
	uint32_t *PSR = (uint32_t *)(current_task -> stack_addr);
	uint32_t *R0 = (uint32_t *)(current_task -> stack_addr - 7*sizeof(uint32_t));
	uint32_t *PC = (uint32_t *)(current_task -> stack_addr - sizeof(uint32_t));
	
	*PSR = 0x01000000;
	*PC = (uint32_t)(task);
	*PC -= 1;
	*R0 = (uint32_t)(arg);
	
	// populate R4-R11
	for (int i = 0; i<8; i++)
	{
		// i = 0,1,2,3,4,5,6,7
		uint32_t *curr = (uint32_t *)(current_task ->stack_addr - (15-i)*sizeof(uint32_t));
		
		*curr = 0x00000000 + (i+1);
	}
	
	for (int i = 9; i<14; i++)
	{
		uint32_t *curr = (uint32_t *)(current_task -> stack_addr - (15-i)*sizeof(uint32_t));
		
		*curr = 0xFFFF0000 + i;
	}
	
	current_task -> stack_addr -= 16*4;
	
}

void t1(void *arg)
{
	// idle task
	while(1)
	{
		Delay(1);
		printf("\nidle %d",msTicks);
	}
}

void t2(void *arg)
{
	while(1)
	{
		Delay(10);
		printf("\nTask 2");
	}
}

void t3(void *arg)
{
	while(1)
	{
		Delay(10);
		printf("\nTask 3");
	}
}

void osKernelStart(void){
	uint32_t *vectorTable = 0x0;
	uint32_t mainStack = vectorTable[0];
	__set_MSP(mainStack);
	
	//Switch from MSP to PSP
	__set_CONTROL(__get_CONTROL() | 0x02);
	__set_PSP(TASKS[0].stack_addr);
	
	currentTask = &TASKS[0];
	stackPointer_current = currentTask -> stack_addr;
	
	NVIC_SetPriority(SysTick_IRQn, 0xff);
	NVIC_SetPriority(PendSV_IRQn, 0x00);

	printf("\nStarting...\n\n");
	
	/*
	uint32_t period = 1000; // 1s
	uint32_t prev = -period;
	
	while(true) {
		if((uint32_t)(msTicks - prev) >= period) {
			printf("tick ");
			prev += period;
		}
	}
	*/
	SysTick_Config(SystemCoreClock/10);
	t1(NULL);
}

int SWITCH = 0;

void SysTick_Handler(void) {
	msTicks++;
	if (msTicks % 10 == 0)
	{
		printf("\n	tick %d",msTicks);
		
		if (SWITCH == 1)
		{
			SWITCH = 0;
			// printf(" - switch to task 1");
		}
		else
		{
			SWITCH = 1;
			// printf(" - switch to task 2");
		}
		readyTask = &TASKS[SWITCH];
		stackPointer_next = readyTask -> stack_addr;
		
		SCB -> ICSR |= 1 << 28;
	}
	
	/*
	readyTask = &TASKS[SWITCH];
	stackPointer_next = readyTask -> stack_addr;
	
	SCB -> ICSR |= 1 << 28;
	*/

}
// TODO
// stackPointer_current will be updated to hold the address of the current task's Stack Pointer
// stackPointer_next will be updated to hold the address of the next ready task's Stack Pointer

__asm void PendSV_Handler(void) {
	MRS R1,MSP
	
	MRS R0,PSP
	MOV R13,R0
	
	/*
	MRS R1,MSP
	
	MRS R0,PSP
	MOV R13,R0
	
	PUSH {R4-R11}
	LDR R4,=__cpp(&stackPointer_current)
	STR R13,[R4]
	
	LDR R4,=__cpp(&stackPointer_next)
	LDR R13,[R4]
	POP {r4-r11}
	
	MOV R0,R13
	MSR PSP,R0
	LDR R0,=__cpp(&stackPointer_next)
	STR R13,[R0]
	
	MOV R13,R1
	*/
	MOV R13,R1
	BX		LR
}

int main(void) {
	// default code
	printf("--- system init ---\n");
	osKernelInitialize();
	osThreadStart(t1,NULL,IDLE);
	osThreadStart(t2,NULL,NORMAL);
	osThreadStart(t3,NULL,NORMAL);
	osKernelStart();

}
