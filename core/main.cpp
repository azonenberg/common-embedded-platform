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

///@brief Key-value store used for storing configuration settings
KVS* g_kvs = nullptr;

///@brief Global log sink object
LogSink<MAX_LOG_SINKS>* g_logSink = nullptr;

//called by newlib
extern "C" void hardware_init_hook();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application entry point

extern "C" void* memcpy_base(void* dst0, const void* src0, size_t len);

extern "C" void hardware_init_hook()
{
	//NOTE: on some platforms we put memcpy in ITCM to make it faster
	//we cannot use memcpy to do this, for obvious reasons
	//so we vendor newlib's memcpy instead (renamed to memcpy_base) and use it here

	//Copy .data from flash to SRAM (for some reason the default newlib startup won't do this??)
	memcpy_base(&__data_start, &__data_romstart, &__data_end - &__data_start + 1);

	#ifdef HAVE_ITCM
		//Copy ITCM code from flash to SRAM
		memcpy_base(&__itcm_start, &__itcm_romstart, &__itcm_end - &__itcm_start + 1);
		asm("dsb");
		asm("isb");
	#endif

	//Initialize the floating point unit
	#if defined(STM32H735) || defined(STM32L431)
		SCB.CPACR |= ((3UL << 20U)|(3UL << 22U));
	#endif
}

int main()
{
	//Re-enable interrupts since the bootloader (if used) may have turned them off
	EnableInterrupts();

	//Enable some core peripherals if we have them (things we're always going to want to use)
	#ifdef STM32L431
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
	BSP_MainLoop();

	//never get here
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global helper functions

void __attribute__((noreturn)) Reset()
{
	SCB.AIRCR = 0x05fa0004;
	while(1)
	{}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Default main loop, may override in special cases

void __attribute__((weak)) BSP_MainLoop()
{
	g_log("Total tasks: %d of %d slots\n", g_tasks.size(), g_tasks.capacity());
	g_log("Timer tasks: %d of %d slots\n", g_timerTasks.size(), g_timerTasks.capacity());
	g_log("Ready\n");

	while(1)
	{
		//Check for overflows on our timer
		const int logTimerMax = 60000;
		if(g_log.UpdateOffset(logTimerMax))
		{
			for(auto t : g_timerTasks)
				t->OnTimerShift(logTimerMax);
		}

		//Run all of our regular tasks
		for(auto t : g_tasks)
			t->Iteration();

		//Run any non-task stuff
		BSP_MainLoopIteration();
	}
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

void __attribute__((weak)) BSP_MainLoopIteration()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers for determining which mode the firmware was built in

bool __attribute__((weak)) IsBootloader()
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global state we use in pretty much every firmware

/**
	@brief Set up the microkvs key-value store for persisting our configuration
 */
void InitKVS(StorageBank* left, StorageBank* right, uint32_t logsize)
{
	g_log("Initializing microkvs key-value store\n");
	static KVS kvs(left, right, logsize);
	g_kvs = &kvs;

	LogIndenter li(g_log);
	g_log("Block size:  %d bytes\n", kvs.GetBlockSize());
	g_log("Log:         %d / %d slots free\n", (int)kvs.GetFreeLogEntries(), (int)kvs.GetLogCapacity());
	g_log("Data:        %d / %d bytes free\n", (int)kvs.GetFreeDataSpace(), (int)kvs.GetDataCapacity());
	g_log("Active bank: %s (rev %d)\n",
		kvs.IsLeftBankActive() ? "left" : "right",
		kvs.GetBankHeaderVersion() );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper for firmware updates etc

void FormatBuildID(const uint8_t* buildID, char* strOut)
{
	const char* hex = "0123456789abcdef";
	for(int i=0; i<20; i++)
	{
		strOut[i*2] = hex[buildID[16+i] >> 4];
		strOut[i*2 + 1] = hex[buildID[16+i] & 0xf];
	}
	strOut[40] = '\0';
}
