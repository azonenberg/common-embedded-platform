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

#include "../core/platform.h"
#include "FMCUtils.h"

#ifdef HAVE_FMC
#include <peripheral/FMC.h>

/**
	@brief Initialize the FMC peripheral in our standard configuration for FPGA interfacing:

	* PLL2 R as clock source
	* 16 bit multiplexed synchronous PSRAM interface
	* Free running clock
	* Synchronous wait states
	* FPGA mapped at 0xc0000000 with MPU configured for device memory (strongly ordered, no caching)
 */
void InitFMCForFPGA()
{
	//Enable the FMC and select PLL2 R as the clock source
	RCCHelper::Enable(&_FMC);
	RCCHelper::SetFMCKernelClock(RCCHelper::FMC_CLOCK_PLL2_R);

	//Use free-running clock output (so FPGA can clock APB off it)
	//Configured as 16-bit multiplexed synchronous PSRAM
	static FMCBank fmc(&_FMC, 0);
	fmc.EnableFreeRunningClock();
	fmc.EnableWrites();
	fmc.SetSynchronous();
	fmc.SetBusWidth(FMC_BCR_WIDTH_16);
	fmc.SetMemoryType(FMC_BCR_TYPE_PSRAM);
	fmc.SetAddressDataMultiplex();

	//Enable wait states wiath NWAIT active during the wait
	fmc.EnableSynchronousWaitStates();
	fmc.SetEarlyWaitState(false);

	//Map the PSRAM bank in slot 1 (0xc0000000) as strongly ordered / device memory
	fmc.SetPsramBankAs1();
}

#endif
