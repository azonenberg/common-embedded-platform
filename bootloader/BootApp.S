.thumb
.syntax unified

.globl DoBootApplication
DoBootApplication:

	//Disable interrupts so we don't have anything fire during the boot flow
	cpsid	i

	//Update VTOR to point to the new vector table
	ldr		r1, pvtor
	str		r0, [r1, #0]

	//Read the new stack pointer address
	ldr		sp, [r0, #0]

	//Jump to the application firmware
	ldr		r1, [r0, #4]
	bx		r1

//constant addresses
.align 4
pvtor:	.word 0xe000ed08
