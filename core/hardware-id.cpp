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

#ifdef STM32H735
#define HAVE_PKG
#endif

#ifdef HAVE_PKG
const char* GetPackage(uint8_t pkg);
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Print identifying hardware info

void __attribute__((weak)) BSP_DetectHardware()
{
	g_log("Identifying hardware\n");
	LogIndenter li(g_log);

	auto srev = GetStepping(DBGMCU.IDCODE >> 16);
	auto part = GetPartName(DBGMCU.IDCODE & 0xfff);

	#ifdef HAVE_PKG
		RCCHelper::EnableSyscfg();

		auto pkg = GetPackage(SYSCFG.PKGR);
		g_log("STM32%s stepping %s, %s\n", part, srev, pkg);
	#else
		g_log("STM32%s stepping %s\n", part, srev);
	#endif

	#ifdef STM32L431
		g_log("64 kB total SRAM, 1 kB EEPROM, 128 byte backup SRAM\n");
	#elif defined(STM32L031)
		g_log("8 kB total SRAM, 1 kB EEPROM, 20 byte backup SRAM\n");
	#elif defined(STM32H735)
		g_log("564 kB total SRAM, 128 kB DTCM, up to 256 kB ITCM, 4 kB backup SRAM\n");
	#endif

	#ifdef STM32L431
		g_log("%d kB Flash\n", FLASH_SIZE);
	#else
		g_log("%d kB Flash\n", F_ID);
	#endif

	uint16_t waferX = U_ID[0] >> 16;
	uint16_t waferY = U_ID[0] & 0xffff;
	uint8_t waferNum = U_ID[1] & 0xff;
	#ifdef STM32H7
		//U_ID fields documented in 45.1 of STM32 programming manual
		char waferLot[8] =
		{
			static_cast<char>((U_ID[1] >> 24) & 0xff),
			static_cast<char>((U_ID[1] >> 16) & 0xff),
			static_cast<char>((U_ID[1] >> 8) & 0xff),
			static_cast<char>((U_ID[2] >> 24) & 0xff),
			static_cast<char>((U_ID[2] >> 16) & 0xff),
			static_cast<char>((U_ID[2] >> 8) & 0xff),
			static_cast<char>((U_ID[2] >> 0) & 0xff),
			'\0'
		};
	#elif defined(STM32L031)
		waferNum = (U_ID[0] >> 24) & 0xff;
		char waferLot[8] =
		{
			static_cast<char>((U_ID[0] >> 16) & 0xff),
			static_cast<char>((U_ID[0] >> 8) & 0xff),
			static_cast<char>((U_ID[0] >> 0) & 0xff),
			static_cast<char>((U_ID[1] >> 24) & 0xff),
			static_cast<char>((U_ID[1] >> 16) & 0xff),
			static_cast<char>((U_ID[1] >> 8) & 0xff),
			static_cast<char>((U_ID[1] >> 0) & 0xff),
			'\0'
		};
		waferX = U_ID[2] >> 16;
		waferY = U_ID[2] & 0xffff;
	#else
		//TODO: double check this is right ordering
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
	#endif
	g_log("Lot %s, wafer %d, die (%d, %d)\n", waferLot, waferNum, waferX, waferY);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Crack device IDs

#ifdef HAVE_PKG
const char* GetPackage(uint8_t pkg)
{
	//For now, this is STM32H735 specific
	switch(pkg)
	{
		case 0:		return "VQFPN68 (industrial)";
		case 1:		return "LQFP100/TFBGA100 (legacy)";
		case 2:		return "LQFP100 (industrial)";
		case 3:		return "TFBGA100 (industrial)";
		case 4:		return "WLCSP115 (industrial)";
		case 5:		return "LQFP144 (legacy)";
		case 6:		return "UFBGA144 (legacy)";
		case 7:		return "LQFP144 (industrial)";
		case 8:		return "UFBGA169 (industrial)";
		case 9:		return "UFBGA176+25 (industrial)";
		case 10:	return "LQFP176 (industrial)";
		default:	return "unknown package";
	}
}
#endif

const char* GetStepping(uint16_t rev)
{
	#if defined(STM32L4)	//L431
		switch(rev)
		{
			case 0x1000:	return "A";
			case 0x1001:	return "Z";
			case 0x2001:	return "Y";
			default:		return "(unknown)";
		}
	#elif defined(STM32L0)	//L031
		switch(rev)
		{
			case 0x1000:	return "A";
			case 0x2000:	return "B";
			case 0x2008:	return "Y";
			case 0x2018:	return "X";
			default:		return "(unknown)";
		}
	#elif defined(STM32H7)	//H735
		switch(rev)
		{
			case 0x1000:	return "A";
			case 0x1001:	return "Z";
			default:		return "(unknown)";
		}
	#endif

	//if we get here, unrecognized
	return "(unknown)";
}

const char* GetPartName([[maybe_unused]] uint16_t device)
{
	#ifdef STM32L4
		switch(device)
		{
			case 0x435:	return "L43xxx/44xxx";
			case 0x462:	return "L45xxx/46xxx";
			case 0x464:	return "L41xxx/42xxx";
			default:	return "(unknown)";
		}
	#elif defined(STM32L0)
		switch(device)
		{
			case 0x425:	return "L031/041";
			default:	return "(unknown)";
		}
	#elif defined(STM32H7)
		//0x483 is H735, but L_ID has text
		static const char id[5] =
		{
			static_cast<char>((L_ID >> 24) & 0xff),
			static_cast<char>((L_ID >> 16) & 0xff),
			static_cast<char>((L_ID >> 8) & 0xff),
			static_cast<char>((L_ID >> 0) & 0xff),
			'\0'
		};
		return id;
	#endif

	//if we get here, unrecognized
	return "(unknown)";
}
