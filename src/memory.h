#ifndef MEMORY_H
#define MEMORY_H

#include <malloc.h>
#include <stdio.h>

uint32_t getTotalHeap(void)
{
	extern char __StackLimit, __bss_end__;
	return &__StackLimit - &__bss_end__;
}

uint32_t getFreeHeap(void)
{
	struct mallinfo m = mallinfo();
	return getTotalHeap() - m.uordblks;
}

void print_memory_status(void)
{
	printf("Total heap: %d, Free heap: %d\n", getTotalHeap(), getFreeHeap());
}

#endif // MEMORY_H
