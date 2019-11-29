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

#define __PRIO

volatile uint32_t msTicks = 0;

int num_tasks = 0;
TCB_t TASKS[6];
int SWITCH = 0;
TCB_t *currentTask, *readyTask;
uint32_t stackPointer_current, stackPointer_next;
bitVector_t bitVector = 0;
queue_t priorityArray[NUM_PRIORITIES];

int fpp_count[] = {0,5,2,5,2,5};
int sem_count = 10;

int contextFlag = 0;

void Delay(uint32_t dlyTicks)
{
	uint32_t curTicks;
	curTicks = msTicks;
	
	while((msTicks - curTicks) < dlyTicks);
}

void prioInherit()
{
	currentTask -> oldPriority = currentTask -> priority;
	currentTask -> priority = HIGH;
}

void prioRestore()
{
	currentTask -> priority = currentTask -> oldPriority;
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
	t->state = READY;
	bitVector |= 1 << t->priority;
}
TCB_t* dequeue(queue_t *q)
{
	TCB_t *curr = q -> head;
	TCB_t *ret = curr;
	
	if (q -> size > 0)
	{
		q->head = curr->next;
		q->size --;
	}
	if(q->size == 0)
	{
		bitVector &= ~(1 << curr->priority);
	}
	return ret;
}

void init_sem(sem_t *sem, uint32_t count)
{
	sem -> s = count;
	queue_init(&(sem -> wait));
	
}
void wait_sem(sem_t *sem)
{
	__disable_irq();
	(sem -> s)--;
	printf("\nt%d req", currentTask -> task_id);
	if (sem -> s < 0)
	{
		currentTask -> state = BLOCKED;
		enqueue(&sem -> wait,currentTask);
		printf("\nt%d wait", currentTask -> task_id);
		contextFlag = 1;
	}
	__enable_irq();
	
	while(contextFlag);
}
void signal_sem(sem_t *sem)
{
	__disable_irq();
	(sem -> s)++;
	if (sem -> s >= 0)
	{
		printf("\nt%d rel",currentTask -> task_id);
		TCB_t *next = dequeue(&sem -> wait);
		next -> state = READY;
		enqueue(&priorityArray[next -> priority],&TASKS[next->task_id]);
		contextFlag = 0;
	}
	__enable_irq();
}

void init_mtx(mutex_t *mtx)
{
	init_sem(&mtx -> m,1);
}
void acquire(mutex_t *mtx)
{
	__disable_irq();
	if (mtx -> m.s == 0)
	{
		// mtx has another owner
		currentTask -> state = BLOCKED;
		TCB_t *blockedTask = dequeue(&priorityArray[currentTask -> priority]);
		enqueue(&mtx -> m.wait,blockedTask);
		printf("\nt%d block", currentTask -> task_id);
		contextFlag = 1;
	} else if (mtx -> m.s == 1)
	{
		// can acquire
		mtx -> m.s = 0;
		mtx -> owner = currentTask -> task_id;
		printf("\nt%d acq", currentTask -> task_id);
		
		#ifdef __PRIO
		prioInherit();
		#endif
	}
	__enable_irq();
	while(contextFlag);
}
void release(mutex_t *mtx)
{
	__disable_irq();
	if (mtx -> m.s == 0 && mtx -> owner == currentTask -> task_id)
	{
		// is owner, can release
		printf("\nt%d rel", currentTask -> task_id);
		mtx -> m.s = 1;
		mtx -> owner = 0;
		contextFlag = 0;
		if (mtx -> m.wait.size > 0)
		{
			TCB_t *next = dequeue(&mtx -> m.wait);
			next -> state = READY;
			enqueue(&priorityArray[next -> priority],&TASKS[next->task_id]);
		}
		#ifdef __PRIO
		prioRestore();
		#endif
	}
	else if (mtx -> m.s == 1)
	{
		// mtx not owned
		printf("\nt%d no owner", currentTask -> task_id);
	}
	else if (mtx -> m.s == 0 && mtx -> owner != currentTask -> task_id)
	{
		// not owner
		printf("\nt%d not owner", currentTask -> task_id);
	}
	__enable_irq();
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
	
	for(int i = 0; i<NUM_PRIORITIES; i++)
	{
		queue_init(&priorityArray[i]);
	}
	
	return true;
}

void osThreadStart(rtosTaskFunc_t task, void *arg, priority_t priority)
{
	printf("\ninit t%d p%d",num_tasks,priority);
	TCB_t *current_task = &TASKS[num_tasks];
	
	current_task -> priority = priority;
	current_task -> oldPriority = priority;
	
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
	
	if (num_tasks > 0)
		enqueue(&priorityArray[TASKS[num_tasks].priority],&TASKS[num_tasks]);
	
	num_tasks++;
}

sem_t sem;
mutex_t mtx;

void t0(void *arg)
{
	while(1)
	{	
		__disable_irq();
		printf("\nIDLE");
		__enable_irq();
		
		Delay(1);
	}
}

void terminate()
{
	int idx = currentTask -> task_id;
	TASKS[idx].state = TERMINATED;
	TASKS[idx].stack_addr = TASKS[0].stack_addr;
	TASKS[idx].priority = IDLE;
}

void t1(void *arg)
{
	while(1)
	{
		if (TASKS[1].state != TERMINATED)
		{
			
			#ifdef __PRIO
			acquire(&mtx);
			for(int i = 0; i < 15; i++)
			{
				__disable_irq();
				printf("\nt1");
				__enable_irq();
				Delay(1);
			}
			release(&mtx);
			TASKS[1].state = TERMINATED;
			#endif
			
			#ifdef __MTX
			acquire(&mtx);
			__disable_irq();
			printf("\nt1 has mtx");
			__enable_irq();
			release(&mtx);
			#endif
			
			#ifdef __SEM
			wait_sem(&sem);
			__disable_irq();
			printf("\nt1 has sem");
			__enable_irq();
			Delay(5);
			signal_sem(&sem);
			#endif
			
			#ifdef __FPP
			__disable_irq();
			printf("\nt1 %d", fpp_count[1]);
			__enable_irq();
			fpp_count[1]--;
			if (fpp_count[1] == 0)
			{
				TASKS[1].state = TERMINATED;
			}
			#endif
			
			#ifdef __CONTEXT
			__disable_irq();
			printf("\nt1");
			__enable_irq();
			#endif
			
			Delay(1);
		}
	}
}


void t2(void *arg)
{
	while(1)
	{
		if (TASKS[2].state != TERMINATED)
		{
			
			#ifdef __PRIO
			__disable_irq();
			printf("\nt2");
			__enable_irq();
			Delay(1);
			#endif
			
			#ifdef __MTX
			Delay(10);
			release(&mtx);
			#endif
			
			#ifdef __SEM
			wait_sem(&sem);
			__disable_irq();
			printf("\nt2 has sem");
			__enable_irq();
			Delay(5);
			signal_sem(&sem);
			#endif
			
			#ifdef __FPP
			__disable_irq();
			printf("\nt2 %d", fpp_count[2]);
			__enable_irq();
			fpp_count[2]--;
			if (fpp_count[2] == 0)
			{
				TASKS[2].state = TERMINATED;
			}
			#endif
			
			#ifdef __CONTEXT
			__disable_irq();
			printf("\nt2");
			__enable_irq();
			#endif
			
			Delay(1);
		}
	}
}

void t3(void *arg)
{
	while(1)
	{
		if (TASKS[3].state != TERMINATED)
		{
			
			#ifdef __PRIO
			acquire(&mtx);
			for (int i = 0 ; i < 5; i++)
			{
				__disable_irq();
				printf("\nt3 %d", i);
				__enable_irq();
				Delay(1);
			}
			release(&mtx);
			#endif
			
			#ifdef __FPP
			__disable_irq();
			printf("\nt3 %d", fpp_count[3]);
			__enable_irq();
			
			fpp_count[3]--;
			if (fpp_count[3] == 0)
			{
				TASKS[3].state = TERMINATED;
			}
			#endif
			
			Delay(1);
		}
	}
}

void t4(void *arg)
{
	while(1)
	{
		if (TASKS[4].state != TERMINATED)
		{
			
			#ifdef __FPP
			__disable_irq();
			printf("\nt4 %d", fpp_count[4]);
			__enable_irq();
			fpp_count[4]--;
			if (fpp_count[4] == 0)
			{
				TASKS[4].state = TERMINATED;
			}
			#endif
			
			Delay(1);
		}
	}
}

void t5(void *arg)
{
	while(1)
	{
		if (TASKS[5].state != TERMINATED)
		{
			
			#ifdef __FPP
			__disable_irq();
			printf("\nt5 %d", fpp_count[5]);
			__enable_irq();
			fpp_count[5]--;
			if (fpp_count[5] == 0)
			{
				TASKS[5].state = TERMINATED;
			}
			#endif
			
			Delay(1);
		}
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
	
	currentTask = &TASKS[0];
	stackPointer_current = currentTask -> stack_addr;
	
	printf("\n\nStarting...\n\n");
	
	SysTick_Config(SystemCoreClock/100);
	t0(NULL);
}

int t2Started = 0;
int t3Started = 0;

void SysTick_Handler(void) {
	msTicks++;
	
	#ifdef __PRIO
	if (msTicks > 3 && !t2Started)
	{
		osThreadStart(t2,NULL,HIGH);
		t2Started = 1;
	}
	if (msTicks > 10 && !t3Started)
	{
		osThreadStart(t3,NULL,NORMAL);
		t3Started = 1;
	}
	#endif
	
	#ifndef __CONTEXT
	if (currentTask -> state != TERMINATED && currentTask -> state != BLOCKED)
		enqueue(&priorityArray[currentTask -> priority],&TASKS[currentTask -> task_id]);
	// find next task to run
	uint8_t nextQueue__idx = 31 - __clz(bitVector);
	TCB_t nextTask = *priorityArray[nextQueue__idx].head;
	
	if (currentTask -> task_id != nextTask.task_id)						// no need to perform context switch if the same task it queued up 
	{
		readyTask = dequeue(&priorityArray[nextQueue__idx]);
		stackPointer_next = readyTask -> stack_addr;
		
		SCB -> ICSR |= 1 << 28;
	} else {
		dequeue(&priorityArray[nextQueue__idx]);
	}
	#endif
	
	#ifdef __CONTEXT
	if (msTicks % 3 == 0)
	{
		
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
	}
	#endif
}

__asm void contextSwitch (void)
{
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
	
	/*
	LDR R1,=0x00000000
	LDR R0,=__cpp(&sysTickFlag)
	*/
	
	BX		LR
}

void PendSV_Handler(void) 
{
	// printf("\nswitch from t%d to t%d",currentTask -> task_id, readyTask -> task_id);
	contextSwitch();
	TASKS[currentTask -> task_id].stack_addr = stackPointer_current;
	TASKS[currentTask -> task_id].state = READY;
	TASKS[readyTask -> task_id].stack_addr = stackPointer_next;
	TASKS[readyTask -> task_id].state = RUNNING;
	currentTask = readyTask;
}

int main(void) {
	// default code
	printf("\n\n\n--- system init ---\n");
	osKernelInitialize();
	osThreadStart(t0,NULL,IDLE);
	
	#ifdef __CONTEXT
	// context switching
	osThreadStart(t1,NULL,NORMAL);
	osThreadStart(t2,NULL,NORMAL);
	osThreadStart(t3,NULL,NORMAL);
	#endif
	
	#ifdef __FPP
	// FPP scheduling
	osThreadStart(t1,NULL,HIGH);
	osThreadStart(t2,NULL,HIGH);
	osThreadStart(t3,NULL,NORMAL);
	osThreadStart(t4,NULL,LOW);
	osThreadStart(t5,NULL,LOW);
	#endif
	
	#ifdef __SEM
	// blocking semaphores
	init_sem(&sem,1);
	osThreadStart(t1,NULL,NORMAL);
	osThreadStart(t2,NULL,NORMAL);
	#endif
	
	#ifdef __MTX
	init_mtx(&mtx);
	osThreadStart(t1,NULL,NORMAL);
	osThreadStart(t2,NULL,NORMAL);
	#endif
	
	#ifdef __PRIO
	init_mtx(&mtx);
	osThreadStart(t1,NULL,LOW);
	#endif
	
	osKernelStart();
	
}
