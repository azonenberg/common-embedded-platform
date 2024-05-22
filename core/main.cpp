/***********************************************************************************************************************
*                                                                                                                      *
* common-embedded-platform                                                                                             *
*                                                                                                                      *
* Copyright (c) 2024 Andrew D. Zonenberg and contributors                                                              *
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

#include "platform.h"

/**
	@file
	@author		Andrew D. Zonenberg
	@brief 		Common main() and globals shared by all users of the platform
 */

///@brief The log instance
Logger g_log;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application entry point

int main()
{
	//Copy .data from flash to SRAM (for some reason the default newlib startup won't do this??)
	memcpy(&__data_start, &__data_romstart, &__data_end - &__data_start + 1);

	//Re-enable interrupts since the bootloader (if used) may have turned them off
	EnableInterrupts();

	//Enable some core peripherals if we have them (things we're always going to want to use)
	#ifdef HAVE_PWR
	RCCHelper::Enable(&PWR);
	#endif

	//Hardware setup
	BSP_InitPower();
	BSP_InitClocks();
	BSP_InitUART();
	BSP_InitLog();
	g_log("Logging ready\n");
	BSP_DetectHardware();

	//Do any other late initialization
	BSP_Init();

	//Main event loop
	g_log("Ready\n");
	while(1)
		BSP_MainLoopIteration();

	//never get here
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Firmware build data string (used by bootloader if we have one)

extern "C" const char
	__attribute__((section(".fwver")))
	__attribute__((used))
	g_firmwareVersion[] = __DATE__ " " __TIME__;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global helper functions

void __attribute__((noreturn)) Reset()
{
	SCB.AIRCR = 0x05fa0004;
	while(1)
	{}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BSP functions (weak dummy implementations, expected to be overridden by application FW)

void __attribute__((weak)) BSP_InitPower()
{
}

void __attribute__((weak)) BSP_InitClocks()
{
}

void __attribute__((weak)) BSP_InitUART()
{
}

void __attribute__((weak)) BSP_InitLog()
{
}

void __attribute__((weak)) BSP_Init()
{
}
