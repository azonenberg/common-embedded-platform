/***********************************************************************************************************************
*                                                                                                                      *
* common-embedded-platform                                                                                             *
*                                                                                                                      *
* Copyright (c) 2023-2025 Andrew D. Zonenberg and contributors                                                         *
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
	@brief Standard BSP overrides used by most if not all STM32H750 projects
 */

#include <core/platform.h>
#include <microkvs/driver/STM32QSPIStorageBank.h>
#include <peripheral/Flash.h>
#include <peripheral/Power.h>
#include <peripheral/RTC.h>
#include "StandardBSP.h"

//APB1 is 118.75 MHz but default is for timer clock to be 2x the bus clock (see table 53 of RM0468)
//Divide down to get 10 kHz ticks
Timer g_logTimer(&TIM2, Timer::FEATURE_GENERAL_PURPOSE, 23750);

void BSP_InitPower()
{
	//Initialize power (must be the very first thing done after reset)
	//H750 doesn't have SMPS so we have to only use the LDO
	Power::ConfigureLDO(RANGE_VOS0);
}

void BSP_InitClocks()
{
	//Configure the flash with wait states and prefetching before making any changes to the clock setup.
	//A bit of extra latency is fine, the CPU being faster than flash is not.
	Flash::SetConfiguration(225, RANGE_VOS0);

	//Switch back to the HSI clock (in case we're already running on the PLL from the bootloader)
	RCCHelper::SelectSystemClockFromHSI();

	//By default out of reset, we're clocked by the HSI clock at 64 MHz
	//Initialize the external clock source at 25 MHz
	RCCHelper::EnableHighSpeedExternalClock();

	//Set up PLL1 to run off the external oscillator
	RCCHelper::InitializePLL(
		1,		//PLL1
		25,		//input is 25 MHz from the HSE
		2,		//25/2 = 12.5 MHz at the PFD
		38,		//12.5 * 36 = 475 MHz at the VCO
		1,		//div P (primary output 475 MHz)
		10,		//div Q (47.5 MHz kernel clock)
		5,		//div R (95 MHz SWO Manchester bit clock, 47.5 Mbps data rate)
		RCCHelper::CLOCK_SOURCE_HSE
	);

	//Set up main system clock tree
	RCCHelper::InitializeSystemClocks(
		1,		//sysclk = 475 MHz (max 480 in VOS0)
		2,		//AHB = 237.5 MHz (max 240)
		2,		//APB1 = 118.75 MHz (max 120)
		2,		//APB2 = 118.75 MHz
		2,		//APB3 = 118.75 MHz
		2		//APB4 = 118.75 MHz
	);

	//RNG clock should be >= HCLK/32
	//AHB2 HCLK is 237.5 MHz so min 7.421875 MHz
	//Select PLL1 Q clock (45 MHz)
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
	//Base address of memory mapped flash
	uint32_t flashBase = 0x90000000;

	g_log("Using external QSPI flash at 0x%08x for microkvs\n", flashBase);
	LogIndenter li(g_log);

	//End of flash (one after the last byte)
	uint32_t flashEnd = flashBase + g_flashQspi.GetFlashSize();

	//Last two sectors
	uint32_t sectorSize = g_flashQspi.GetSectorSize();
	uint32_t sectorA = flashEnd - sectorSize;
	uint32_t sectorB = flashEnd - (sectorSize * 2);
	g_log("Banks at %08x and %08x (%d kB sectors)\n", sectorA, sectorB, sectorSize / 1024);

	//Each log entry is 64 bytes, and we want to allocate ~50% of storage to the log since our objects are pretty
	//small (SSH keys, IP addresses, etc).
	uint32_t halfSectorSize = sectorSize / 2;
	uint32_t logEntrySize = 64;
	uint32_t numLogEntries = halfSectorSize / logEntrySize;
	g_log("Allocating %u kB to %u log entries\n", halfSectorSize / 1024, numLogEntries);

	//Create the storage banks and initialize it
	static STM32QSPIStorageBank left(g_flashQspi, reinterpret_cast<uint8_t*>(sectorA), sectorSize);
	static STM32QSPIStorageBank right(g_flashQspi, reinterpret_cast<uint8_t*>(sectorB), sectorSize);
	InitKVS(&left, &right, numLogEntries);
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
