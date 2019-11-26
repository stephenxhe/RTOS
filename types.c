#include <LPC17xx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t sem_t;
typedef sem_t mutex_t;
typedef void (*rtosTaskFunc_t)(void *args);
typedef uint8_t bitVector_t;

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

typedef struct queue{
	TCB_t *head;
	uint32_t size;
}queue_t;

