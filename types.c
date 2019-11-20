#include <LPC17xx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct OS_TCB {
	uint8_t task_id;
	uint8_t state;
	uint32_t *tsk_stack;
	uint8_t priority;
	uint16_t events;
} *P_TCB;


struct OS_TASK {
	uint8_t task_id;
	uint8_t status;
	
} task;