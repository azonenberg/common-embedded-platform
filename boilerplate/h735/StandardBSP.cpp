/***********************************************************************************************************************
*                                                                                                                      *
* common-embedded-platform                                                                                             *
*                                                                                                                      *
* Copyright (c) 2023-2024 Andrew D. Zonenberg and contributors                                                         *
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

/**
	@brief Standard BSP overrides used by most if not all STM32H735 projects
 */

#include <core/platform.h>
#include <microkvs/driver/STM32StorageBank.h>
#include <peripheral/Flash.h>
#include <peripheral/Power.h>
#include <peripheral/RTC.h>
#include "StandardBSP.h"

//APB1 is 62.5 MHz but default is for timer clock to be 2x the bus clock (see table 53 of RM0468)
//Divide down to get 10 kHz ticks
Timer g_logTimer(&TIM2, Timer::FEATURE_GENERAL_PURPOSE, 12500);

void BSP_InitPower()
{
	//Initialize power (must be the very first thing done after reset)
	Power::ConfigureSMPSToLDOCascade(Power::VOLTAGE_1V8, RANGE_VOS0);
}

void BSP_InitClocks()
{
	//With CPU_FREQ_BOOST not set, max frequency is 520 MHz

	//Configure the flash with wait states and prefetching before making any changes to the clock setup.
	//A bit of extra latency is fine, the CPU being faster than flash is not.
	Flash::SetConfiguration(513, RANGE_VOS0);

	//By default out of reset, we're clocked by the HSI clock at 64 MHz
	//Initialize the external clock source at 25 MHz
	RCCHelper::EnableHighSpeedExternalClock();

	//Set up PLL1 to run off the external oscillator
	RCCHelper::InitializePLL(
		1,		//PLL1
		25,		//input is 25 MHz from the HSE
		2,		//25/2 = 12.5 MHz at the PFD
		40,		//12.5 * 40 = 500 MHz at the VCO
		1,		//div P (primary output 500 MHz)
		10,		//div Q (50 MHz kernel clock)
		10,		//div R (50 MHz SWO Manchester bit clock, 25 Mbps data rate)
		RCCHelper::CLOCK_SOURCE_HSE
	);

	//Set up PLL2 to run the external memory bus
	//We have some freedom with how fast we clock this!
	//Doesn't have to be a multiple of 500 since separate VCO from the main system
	RCCHelper::InitializePLL(
		2,		//PLL2
		25,		//input is 25 MHz from the HSE
		2,		//25/2 = 12.5 MHz at the PFD
		16,		//12.5 * 16 = 200 MHz at the VCO
		32,		//div P (not used for now)
		32,		//div Q (not used for now)
		1,		//div R (200 MHz FMC kernel clock = 100 MHz FMC clock)
		RCCHelper::CLOCK_SOURCE_HSE
	);

	//Set up main system clock tree
	RCCHelper::InitializeSystemClocks(
		1,		//sysclk = 500 MHz
		2,		//AHB = 250 MHz
		4,		//APB1 = 62.5 MHz
		4,		//APB2 = 62.5 MHz
		4,		//APB3 = 62.5 MHz
		4		//APB4 = 62.5 MHz
	);

	//RNG clock should be >= HCLK/32
	//AHB2 HCLK is 250 MHz so min 7.8125 MHz
	//Select PLL1 Q clock (50 MHz)
	RCC.D2CCIP2R = (RCC.D2CCIP2R & ~0x300) | (0x100);

	//Select PLL1 as system clock source
	RCCHelper::SelectSystemClockFromPLL1();
}

void BSP_InitLog()
{
	static LogSink<MAX_LOG_SINKS> sink(&g_cliUART);
	g_logSink = &sink;

	g_log.Initialize(g_logSink, &g_logTimer);
	g_log("Firmware compiled at %s on %s\n", __TIME__, __DATE__);
}

void DoInitKVS()
{
	/*
		Use sectors 6 and 7 of main flash (in single bank mode) for a 128 kB microkvs

		Each log entry is 64 bytes, and we want to allocate ~50% of storage to the log since our objects are pretty
		small (SSH keys, IP addresses, etc). A 1024-entry log is a nice round number, and comes out to 64 kB or 50%,
		leaving the remaining 64 kB or 50% for data.
	 */
	static STM32StorageBank left(reinterpret_cast<uint8_t*>(0x080c0000), 0x20000);
	static STM32StorageBank right(reinterpret_cast<uint8_t*>(0x080e0000), 0x20000);
	InitKVS(&left, &right, 1024);
}

void InitRTCFromHSE()
{
	g_log("Initializing RTC...\n");
	LogIndenter li(g_log);
	g_log("Using external clock divided by 50 (500 kHz)\n");

	//Turn on the RTC APB clock so we can configure it, then set the clock source for it in the RCC
	RCCHelper::Enable(&_RTC);
	RTC::SetClockFromHSE(50);
}
