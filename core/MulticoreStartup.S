/***********************************************************************************************************************
*                                                                                                                      *
* common-embedded-platform                                                                                             *
*                                                                                                                      *
* Copyright (c) 2024-2025 Andrew D. Zonenberg and contributors                                                         *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

#ifdef MULTICORE
#ifdef __aarch64__

//Jump to reset vector at start of image
.section .resetptr
	b	_start				//jump to entry point at start of image so mkfsbl knows where our entry point is

	//VBAR_ELx must be aligned to a 2K boundary
.section .vector
.balign 2048
_vectors:
	b	exceptionHandler	//synchronous exception in EL0
.balign	128
	b	exceptionHandler	//IRQ in EL0
.balign	128
	b	exceptionHandler	//FIQ in EL0
.balign	128
	b	exceptionHandler	//SError in EL0

.balign	128
	b	exceptionHandler	//synchronous exception in ELx
.balign	128
	b	exceptionHandler	//IRQ in ELx
.balign	128
	b	exceptionHandler	//FIQ in ELx
.balign	128
	b	exceptionHandler	//SError in ELx

.balign	128
	b	exceptionHandler	//synchronous exception from lower EL in aarch64
.balign	128
	b	exceptionHandler	//IRQ from lower EL in aarch64
.balign	128
	b	exceptionHandler	//FIQ from lower EL in aarch64
.balign	128
	b	exceptionHandler	//SError from lower EL in aarch64

.balign	128
	b	exceptionHandler	//synchronous exception from lower EL in aarch32
.balign	128
	b	exceptionHandler	//IRQ from lower EL in aarch32
.balign	128
	b	exceptionHandler	//FIQ from lower EL in aarch32
.balign	128
	b	exceptionHandler	//SError from lower EL in aarch32

exceptionHandler:
	b	exceptionHandler

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Variables used by startup
.data
.balign	8
__startupInitStage:
	.quad 0

.text

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reset vector

//Add a padding nop before _start because otherwise we get __dtor_end in the same spot and objdump gets cranky
	nop

.globl _start
_start:

	//Clear GPRs x0-x30
	mov		x0, 0
	mov		x1, 0
	mov		x2, 0
	mov		x3, 0
	mov		x4, 0
	mov		x5, 0
	mov		x6, 0
	mov		x7, 0
	mov		x8, 0
	mov		x9, 0
	mov		x10, 0
	mov		x11, 0
	mov		x12, 0
	mov		x13, 0
	mov		x14, 0
	mov		x15, 0
	mov		x16, 0
	mov		x17, 0
	mov		x18, 0
	mov		x19, 0
	mov		x20, 0
	mov		x21, 0
	mov		x22, 0
	mov		x23, 0
	mov		x24, 0
	mov		x25, 0
	mov		x26, 0
	mov		x27, 0
	mov		x28, 0
	mov		x29, 0
	mov		x30, 0

	//Clear out exception status registers
	msr		ELR_EL1, x0
	msr		ESR_EL1, x0
	msr		SPSR_EL1, x0

	msr		ELR_EL2, x0
	msr		ESR_EL2, x0
	msr		SPSR_EL2, x0

	msr		ELR_EL3, x0
	msr		ESR_EL3, x0
	msr		SPSR_EL3, x0

	//Seems like ROM code enables SMP already, so nothing needed for that

	//L1 is not active yet so we need to turn that on still
	//ROM code invalidates the L1 on reset by default, so we don't have to do that ourself, just enable it

	//TODO: initialize FPU and clear its registers
	//TODO: clear NEON/vector registers

	//TODO: bring up main CPU PLL vs running at whatever freq we do in startup code (this should probably be done in C++)

	//Set up initial exception vector table
	adr		x0, _vectors
	msr		VBAR_EL3, x0

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Figure out which core we're on

.IdentifyCores:

	//Get the core number
	mrs		x19, MPIDR_EL1

	//Initialize stack pointer for each core
	and		x19, x19, 0xff
	cbnz	x19, .InitializeCore1

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Core 0 initialization (fall through from identifyCores)

.InitializeCore0:

	//Initialize stack pointer. apparently aarch64 doesn't let you ldr directly to sp?
	ldr		x0, =__core0stackstart
	mov		sp, x0

	//Call global constructors and do hardware init
	bl		__libc_init_array

	//Call application init for core 0 before other cores, so we can zero BSS and whatever else we need
	mov		x0, x19
	bl		CoreInit

	//Tell other cores constructors are done
	ldr		x0, =__startupInitStage
	mov		x1, #1
	str		x1, [x0]

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common initialization

.CoreInitDone:

	//Call the main function
	mov		x0, x19
	bl		CoreMain

	//If application main loop ever terminates, hang here
.CoreMainTerminated:
	b		.CoreMainTerminated

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Core 1 initialization

.InitializeCore1:

	//Initialize stack pointer
	ldr		x0, =__core1stackstart
	mov		sp, x0

	//Spin until core 0 finishes initializing
	ldr		x0, =__startupInitStage
.core1InitLoop:
	ldr		x1, [x0]
	cbz		x1, .core1InitLoop

	//Initialize the MMU
	bl		BSP_InitMMU

	//Application init
	mov		x0, x19
	bl		CoreInit

	//Done, start the application code
	b		.CoreInitDone

#endif
#endif
