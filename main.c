/*
 * Default main.c for rtos lab.
 * @author Andrew Morton, 2018
 */
#include <LPC17xx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "types.c"

#define STACK_SIZE	1024
#define NUM_PRIORITIES 5

#define FPP

int num_tasks = 0;
TCB_t TASKS[6];
int SWITCH = 0;
TCB_t *currentTask, *readyTask;
volatile uint32_t msTicks = 0;
uint32_t stackPointer_current, stackPointer_next;
bitVector_t bitVector = 0;
queue_t priorityArray[NUM_PRIORITIES];

int fpp_count1 = 0;		// t2 work
int fpp_count2 = 0;		// t3 work
int fpp_count3 = 0;		// t4 work
int fpp_count4 = 0;		// t5 work
int fpp_count5 = 0;		// t6 work

void Delay(uint32_t dlyTicks)
{
	uint32_t curTicks;
	curTicks = msTicks;
	
	while((msTicks - curTicks) < dlyTicks);
}


void init_sem(sem_t *s, uint32_t count)
{
	*s = count;
}
void wait_sem(sem_t *s)
{
	__disable_irq();
	while(*s==0)
	{
		__enable_irq();
		__disable_irq();
	}
	(*s)--;
	__enable_irq();
}
void signal_sem(sem_t *s)
{
	__disable_irq();
	(*s)++;
	__enable_irq();
}

void mutex_init(mutex_t *m)
{
	init_sem(m,1);
}
void acquire(mutex_t *m)
{
	wait_sem(m);
}
void release(mutex_t *m)
{
	signal_sem(m);
}

void queue_init(queue_t *q)
{
	q -> head = NULL;
	q -> size = 0;
}
void enqueue(queue_t *q, TCB_t *t)
{
	TCB_t *curr = q -> head;
	if (q -> size == 0)
	{
		q -> head = t;
	}
	else
	{
		while(curr -> next != NULL)
		{
			curr = curr -> next;
		}
		curr -> next = t;
	}
	q -> size++;
	t->next = NULL;				// after enqueuing, the added TCB's next pointer should be NULL
	bitVector |= 1 << t->priority;
}
void dequeue(queue_t *q)
{
	TCB_t *curr = q -> head;
	if (q -> size > 0)
	{
		q->head = curr->next;
		q->size --;
	}
	
	if(q->size == 0)
	{
		bitVector &= ~(1 << curr->priority);
	}
}


bool osKernelInitialize(void)
{
	uint32_t *vectorTable = 0x0;
	uint32_t mainStack = vectorTable[0];
	// initialize each TCB with the base address for its stack
	
	for(int i = 0; i<6; i++)
	{
		// i = 0,1,2,3,4,5
		
		int task_id = 5-i;
		TASKS[task_id].stack_addr =  mainStack - 2048 - 1024*i;
		TASKS[task_id].task_id = task_id;
		TASKS[task_id].state = INACTIVE;
		TASKS[task_id].priority = IDLE;
	}
	
	printf("\nmain stack starting at 0x%x",mainStack);
	for (int i = 0; i<6; i++)
	{
		printf("\ntask %d stack starting at 0x%x",i, TASKS[i].stack_addr);
	}
	
	return true;
}

void osThreadStart(rtosTaskFunc_t task, void *arg, priority_t priority)
{
	printf("\ninit t%d",num_tasks);
	TCB_t *current_task = &TASKS[num_tasks];
	
	current_task -> priority = priority;
	
	uint32_t *PSR = (uint32_t *)(current_task -> stack_addr);
	uint32_t *R0 = (uint32_t *)(current_task -> stack_addr - 7*sizeof(uint32_t));
	uint32_t *PC = (uint32_t *)(current_task -> stack_addr - sizeof(uint32_t));
	
	*PSR = 0x01000000;
	
	*PC = (uint32_t)(task);
	
	*R0 = (uint32_t)(arg);
	
	// populate R4-R11
	for (int i = 0; i<8; i++)
	{
		// i = 0,1,2,3,4,5,6,7
		uint32_t *curr = (uint32_t *)(current_task ->stack_addr - (15-i)*sizeof(uint32_t));
		
		*curr = 0xAB000001 + num_tasks;
	}
	
	for (int i = 9; i<14; i++)
	{
		uint32_t *curr = (uint32_t *)(current_task -> stack_addr - (15-i)*sizeof(uint32_t));
		
		*curr = 0xFFFF0001 + num_tasks;
	}
	
	current_task -> stack_addr -= 15*4;
	
	num_tasks++;
}


void t1(void *arg)
{
	while(1)
	{
		
		#ifdef FPP
		
		#endif
		
		#ifdef CONTEXT
		TASKS[currentTask -> task_id].stack_addr = stackPointer_current;
		TASKS[0].stack_addr = __get_PSP();
		currentTask = &TASKS[0];
		stackPointer_current = currentTask -> stack_addr;
		
		printf("\nTask0 %d", msTicks);
		
		Delay(1);
		#endif
		
	}
}


void t2(void *arg)
{
	while(1)
	{
		
		#ifdef CONTEXT
		TASKS[currentTask -> task_id].stack_addr = stackPointer_current;
		TASKS[1].stack_addr = __get_PSP();
		currentTask = &TASKS[1];
		stackPointer_current = currentTask -> stack_addr;
		
		printf("\nTask1 %d", msTicks);
		
		Delay(1);
		#endif
		
	}
}


void t3(void *arg)
{
	while(1)
	{
		
		#ifdef CONTEXT
		TASKS[currentTask -> task_id].stack_addr = stackPointer_current;
		TASKS[2].stack_addr = __get_PSP();
		currentTask = &TASKS[2];
		stackPointer_current = currentTask -> stack_addr;
		
		printf("\nTask2 %d", msTicks);
		
		Delay(1);
		#endif
		
	}
}


void t4(void *arg)
{
	while(1)
	{
		
		#ifdef CONTEXT
		TASKS[currentTask -> task_id].stack_addr = stackPointer_current;
		TASKS[3].stack_addr = __get_PSP();
		currentTask = &TASKS[3];
		stackPointer_current = currentTask -> stack_addr;
		
		printf("\nTask3 %d", msTicks);
		
		Delay(1);
		#endif
		
	}
}

void t5(void *arg)
{
	while(1)
	{
		
		#ifdef CONTEXT
		TASKS[currentTask -> task_id].stack_addr = stackPointer_current;
		TASKS[3].stack_addr = __get_PSP();
		currentTask = &TASKS[3];
		stackPointer_current = currentTask -> stack_addr;
		
		printf("\nTask4 %d", msTicks);
		
		Delay(1);
		#endif
		
	}
}

void t6(void *arg)
{
	while(1)
	{
		
		#ifdef CONTEXT
		TASKS[currentTask -> task_id].stack_addr = stackPointer_current;
		TASKS[3].stack_addr = __get_PSP();
		currentTask = &TASKS[3];
		stackPointer_current = currentTask -> stack_addr;
		
		printf("\nTask5 %d", msTicks);
		
		Delay(1);
		#endif
		
	}
}

void init_priorityArray()
{
	printf("\n\n--- init priority array ---\n");
	for(int i = 0; i<NUM_PRIORITIES; i++)
	{
		queue_init(&priorityArray[i]);
	}
	osThreadStart(t1,NULL,IDLE);
	osThreadStart(t2,NULL,LOW);
	osThreadStart(t3,NULL,LOW);
	osThreadStart(t4,NULL,NORMAL);
	osThreadStart(t5,NULL,HIGH);
	osThreadStart(t6,NULL,HIGH);
	
	for(int i = 0; i<6; i++)
	{
		enqueue(&priorityArray[TASKS[i].priority],&TASKS[i]);
	}
}

void osKernelStart(void)
{
	uint32_t *vectorTable = 0x0;
	uint32_t mainStack = vectorTable[0];
	__set_MSP(mainStack);
	
	//Switch from MSP to PSP
	__set_CONTROL(__get_CONTROL() | 0x02);
	__set_PSP(TASKS[0].stack_addr);
	
	NVIC_SetPriority(SysTick_IRQn, 0x00);
	NVIC_SetPriority(PendSV_IRQn, 0xff);

	printf("\n\nStarting...\n\n");
	
	SysTick_Config(SystemCoreClock/10);
	t1(NULL);
}


void SysTick_Handler(void) {
	msTicks++;
	
	#ifdef FPP
	int leadingZ = __clz(bitVector);
	
	#endif
	
	#ifdef CONTEXT
	// context switching
	if (msTicks % 5 == 0)
	{
		printf("\n	switch from task %d",SWITCH+1);
		
		if (SWITCH == 0)
		{
			SWITCH = 2;
		}
		else if (SWITCH == 2)
		{
			SWITCH = 1;
		}
		else if (SWITCH == 1)
		{
			SWITCH = 0;
		}
		
		readyTask = &TASKS[SWITCH];
		stackPointer_next = readyTask -> stack_addr;
		
		SCB -> ICSR |= 1 << 28;
		printf(" to task %d", SWITCH+1);
	}
	#endif

}

__asm void PendSV_Handler(void) {
	MRS R1,MSP
	MRS R0,PSP
	
	STMFD R0!,{R4-R11}
	LDR R4,=__cpp(&stackPointer_current)
	STR R0,[R4]
	
	LDR R4,=__cpp(&stackPointer_next)
	LDR R0,[R4]
	LDMFD R0!,{R4-R11}
	
	MSR PSP,R0
	MSR MSP,R1
	
	BX		LR
}

int main(void) {
	// default code
	printf("\n\n\n--- system init ---\n");
	
	#ifdef CONTEXT
	// context switching
	osKernelInitialize();
	osThreadStart(t1,NULL,IDLE);
	osThreadStart(t2,NULL,NORMAL);
	osThreadStart(t3,NULL,NORMAL);
	osThreadStart(t4,NULL,NORMAL);
	#endif
	
	
	#ifdef FPP
	// FPP scheduling
	osKernelInitialize();
	init_priorityArray();
	#endif
	
	osKernelStart();
	
}
