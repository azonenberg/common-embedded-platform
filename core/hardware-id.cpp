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

const char* GetStepping(uint16_t rev);
const char* GetPartName(uint16_t device);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Print identifying hardware info

void __attribute__((weak)) BSP_DetectHardware()
{
	g_log("Identifying hardware\n");
	LogIndenter li(g_log);

	auto srev = GetStepping(DBGMCU.IDCODE >> 16);
	auto part = GetPartName(DBGMCU.IDCODE & 0xfff);

	g_log("STM32%s stepping %s\n", part, srev);

	#ifdef STM32L431
	g_log("64 kB total SRAM, 1 kB EEPROM, 128 byte backup SRAM\n");
	#endif

	g_log("%d kB Flash\n", FLASH_SIZE);

	uint16_t waferX = U_ID[0] >> 16;
	uint16_t waferY = U_ID[0] & 0xffff;
	uint8_t waferNum = U_ID[1] & 0xff;
	char waferLot[8] =
	{
		static_cast<char>((U_ID[2] >> 24) & 0xff),
		static_cast<char>((U_ID[2] >> 16) & 0xff),
		static_cast<char>((U_ID[2] >> 8) & 0xff),
		static_cast<char>((U_ID[2] >> 0) & 0xff),
		static_cast<char>((U_ID[1] >> 24) & 0xff),
		static_cast<char>((U_ID[1] >> 16) & 0xff),
		static_cast<char>((U_ID[1] >> 8) & 0xff),
		'\0'
	};
	g_log("Lot %s, wafer %d, die (%d, %d)\n", waferLot, waferNum, waferX, waferY);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Crack device IDs

const char* GetStepping(uint16_t rev)
{
	#if defined(STM32L4)
		switch(rev)
		{
			case 0x1000:	return "A";
			case 0x1001:	return "Z";
			case 0x2001:	return "Y";
			default:		return "(unknown)";
		}
	#endif

	//if we get here, unrecognized
	return "(unknown)";
}

const char* GetPartName(uint16_t device)
{
	#ifdef STM32L4
		switch(device)
		{
			case 0x435:	return "L43xxx/44xxx";
			case 0x462:	return "L45xxx/46xxx";
			case 0x464:	return "L41xxx/42xxx";
			default:	return "(unknown)";
		}
	#endif

	//if we get here, unrecognized
	return "(unknown)";
}
