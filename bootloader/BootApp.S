.thumb
.syntax unified

.globl DoBootApplication
DoBootApplication:

	//Barriers to make sure pending stuff has committed
	dmb st
	isb

	//Disable interrupts so we don't have anything fire during the boot flow
	cpsid	i

	//Disable caches, if we have them
#ifdef STM32H735
	push	{r0}
	ldr		r1, =DisableInstructionCache
	blx		r1
	ldr		r1, =DisableDataCache
	blx		r1
	ldr		r1, =InvalidateInstructionCache
	blx		r1
	ldr		r1, =InvalidateDataCache
	blx		r1
	pop		{r0}
#endif

	//Update VTOR to point to the new vector table
	ldr		r1, pvtor
	str		r0, [r1, #0]

	//Read the new stack pointer address
	ldr		r2, [r0, #0]
	mov		sp, r2

	//Commit pending operations
	dmb		st
	isb

	//Jump to the application firmware
	ldr		r1, [r0, #4]
	bx		r1

//constant addresses
.align 4
pvtor:	.word 0xe000ed08
