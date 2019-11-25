/*
 * Default main.c for rtos lab.
 * @author Andrew Morton, 2018
 */
#include <LPC17xx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define STACK_SIZE	1024
#define NUM_PRIORITIES 5

typedef void (*rtosTaskFunc_t)(void *args);

typedef uint32_t sem_t;
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

typedef sem_t mutex_t;
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

typedef enum{
	READY,
	RUNNING,
	BLOCKED,
	INACTIVE,
	TERMINATED
} state_t;

typedef enum{
	IDLE = 0x00,
	LOW = 0x1,
	NORMAL = 0x2,
	ABOVE_NORMAL = 0x3,
	HIGH = 0x4
}priority_t;

typedef struct TCB{
	uint8_t task_id;
	state_t state;
	uint32_t stack_addr;
	priority_t priority;
	struct TCB *next;
} TCB_t;

typedef uint32_t bitVector_t;
bitVector_t bitVector = 0;

typedef struct queue{
	TCB_t *head;
	uint32_t size;
}queue_t;
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

queue_t priorityArray[NUM_PRIORITIES];
void init_priorityArray()
{
	for(int i = 0; i<NUM_PRIORITIES; i++)
	{
		queue_init(&priorityArray[i]);
	}
}

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
	printf("\ninit task %d",num_tasks);
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
		
		*curr = 0xAB000000 + num_tasks;
	}
	
	for (int i = 9; i<14; i++)
	{
		uint32_t *curr = (uint32_t *)(current_task -> stack_addr - (15-i)*sizeof(uint32_t));
		
		*curr = 0xFFFF0000 + num_tasks;
	}
	
	current_task -> stack_addr -= 15*4;
	
	enqueue(&priorityArray[priority],current_task);
	
	num_tasks++;
}

void t1(void *arg)
{
	while(1)
	{
		TASKS[currentTask -> task_id].stack_addr = stackPointer_current;
		TASKS[0].stack_addr = __get_PSP();
		currentTask = &TASKS[0];
		stackPointer_current = currentTask -> stack_addr;
		
		printf("\nTask0");
		Delay(1);
	}
}

void t2(void *arg)
{
	while(1)
	{
		TASKS[currentTask -> task_id].stack_addr = stackPointer_current;
		TASKS[1].stack_addr = __get_PSP();
		currentTask = &TASKS[1];
		stackPointer_current = currentTask -> stack_addr;
		
		printf("\nTask1");
		Delay(1);
	}
}

void t3(void *arg)
{
	while(1)
	{
		TASKS[currentTask -> task_id].stack_addr = stackPointer_current;
		TASKS[2].stack_addr = __get_PSP();
		currentTask = &TASKS[2];
		stackPointer_current = currentTask -> stack_addr;
		
		printf("\nTask2");
		Delay(1);
	}
}

void osKernelStart(void){
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

int SWITCH = 0;

void SysTick_Handler(void) {
	msTicks++;
	if (msTicks % 10 == 0)
	{
		printf("\n	switch from task %d",SWITCH+1);
		
		if (SWITCH == 0)
		{
			SWITCH = 1;
		}
		else if (SWITCH == 1)
		{
			SWITCH = 2;
		}
		else if (SWITCH == 2)
		{
			SWITCH = 0;
		}
		
		readyTask = &TASKS[SWITCH];
		stackPointer_next = readyTask -> stack_addr;
		
		SCB -> ICSR |= 1 << 28;
		printf(" to task %d", SWITCH+1);
	}
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
	
	osKernelInitialize();
	init_priorityArray();
	osThreadStart(t1,NULL,IDLE);
	osThreadStart(t2,NULL,NORMAL);
	osThreadStart(t3,NULL,NORMAL);
	osKernelStart();

}
