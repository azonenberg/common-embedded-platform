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

#include "platform.h"

#ifdef HAVE_BSEC
#include <peripheral/BSEC.h>
#endif

#ifdef HAVE_ICACHE
#include <peripheral/ICACHE.h>
#endif

#ifdef HAVE_DCACHE
#include <peripheral/DCACHE.h>
#endif

const char* GetStepping(uint16_t rev);
const char* GetPartName(
	#ifdef STM32MP2
		uint32_t device
	#else
		uint16_t device
	#endif
);

#ifdef HAVE_PKG
const char* GetPackage(uint8_t pkg);
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Print info about an ARM CPU

#ifdef __aarch64__

void PrintCortexAInfo()
{
	//TODO: identify stuff
}

//#elseif defined(__arm__)
#else

void PrintCortexMInfo()
{
	const char* vendor = "(Unknown)";
	switch(SCB.CPUID >> 24)
	{
		case 0x41:
			vendor = "ARM";
			break;
		default:
			break;
	}

	uint32_t major = (SCB.CPUID >> 20) & 0xf;
	uint32_t minor = SCB.CPUID & 0xf;

	const char* part = "(Unknown)";
	switch((SCB.CPUID >> 4) & 0xfff)
	{
		case 0xc24:
			part = "Cortex-M4";
			break;

		case 0xc27:
			part = "Cortex-M7";
			break;

		case 0xd21:
			part = "Cortex-M33";
			break;
	}

	//not currently in linker script
	uint32_t revidr = *reinterpret_cast<uint32_t*>(0xe000ecfc);

	g_log("%s %s revision %d patch %d rev %d\n", vendor, part, major, minor, revidr);

	LogIndenter li(g_log);
	uint32_t ras = (SCB.ID_PFR0 >> 28);
	uint32_t state1 = (SCB.ID_PFR0 >> 4) & 0xf;
	g_log("RAS extension: %s\n", (ras == 2) ? "version 1" : "not available");
	g_log("ID_AFR0:       %08x\n", SCB.ID_AFR0);
	g_log("ID_DFR0:       %08x\n", SCB.ID_DFR0);
	g_log("PACBTI:        %x\n", (SCB.ID_ISAR[5] >> 20) & 0xf);
	g_log("MPU aux ctl:   %s\n", ((SCB.ID_MMFR[0] >> 20) & 0xf) == 1 ? "available" : "not available");
	g_log("TCM:           %s\n", ((SCB.ID_MMFR[0] >> 16) & 0xf) == 1 ? "available" : "not available");
	g_log("CLIDR:         %08x\n", SCB.CLIDR);
	g_log("CTR:           %08x\n", SCB.CTR);
	//don't bother printing T32/A32 flags

	//TODO: check FPU and whatever else
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Print identifying hardware info

#ifdef STM32MP2

void PrintFuseInfo();
void PrintIcacheInfo();
void PrintDcacheInfo();

//MP2 has completely different efuse based identification scheme
void __attribute__((weak)) BSP_DetectHardware()
{
	g_log("Identifying hardware\n");
	LogIndenter li(g_log);

	//Check boot mode
	#ifdef STM32MP2_CPU2
		uint32_t thisCPU = 2;
	#else
		uint32_t thisCPU = 1;
	#endif
	uint32_t bootCPU = 0;
	const char* bootMode = "(unknown)";
	switch(SYSCFG.BOOTSR)
	{
		//Development boot: CPU1 is boot CPU
		case BOOT_MODE_DEV1:
		case BOOT_MODE_DEV2:
			bootMode = "development";
			bootCPU = 1;
			break;

		//M33-TD modes
		case BOOT_MODE_M33TD_SPI:
			bootMode = "M33-TD from SPI flash";
			bootCPU = 2;
			break;

		default:
			break;
	}
	g_log("Boot mode: 0x%x (%s)\n", SYSCFG.BOOTSR, bootMode);

	#ifdef STM32MP2_CPU1
		g_log("Running on CPU1\n");
		PrintCortexAInfo();
	#endif

	#ifdef STM32MP2_CPU2
		g_log("Running on CPU2\n");
		{
			LogIndenter li2(g_log);
			PrintCortexMInfo();
			PrintIcacheInfo();
			PrintDcacheInfo();
		}
	#endif

	if(thisCPU == bootCPU)
		PrintFuseInfo();
}

void PrintIcacheInfo()
{
	#ifdef HAVE_ICACHE
		g_log("External AHB L1 instruction cache present\n");
		LogIndenter li(g_log);

		g_log("Cache IPIDR 0x%08x, version %d.%d\n",
			_ICACHE.IPIDR,
			(_ICACHE.VERR >> 4) & 0xf,
			_ICACHE.VERR & 0xf);
		g_log("ECC available:    %s\n", (_ICACHE.HWCFGR & ICACHE_HWCFGR_ECC) ? "yes" : "no");
		g_log("AHBS interface:   %d bits\n", ICACHE::GetAHBSWidth());
		g_log("AHBM1 interface:  %d bits\n", ICACHE::GetAHBM1Width());
		g_log("AHBM2 interface:  %d bits\n", ICACHE::GetAHBM2Width());
		g_log("Remap capability: %d regions of %d MB\n", ICACHE::GetRemapRegions(), ICACHE::GetRemapRegionSize());
		g_log("Line width:       %d bytes\n", ICACHE::GetCacheLineWidth());
		g_log("Cache size:       %d kB\n", ICACHE::GetCacheSize());
		g_log("Associativity:    %d way\n", ICACHE::GetNumWays());
		g_log("Current status:   %s\n", ICACHE::IsEnabled() ? "enabled" : "disabled");
		g_log("Hits:             %u\n", ICACHE::PerfGetHitCount());
		g_log("Misses:           %u\n", ICACHE::PerfGetMissCount());
	#endif
}

void PrintDcacheInfo()
{
	#ifdef HAVE_DCACHE
		g_log("External AHB L1 data cache present\n");
		LogIndenter li(g_log);

		g_log("Cache IPIDR 0x%08x, version %d.%d\n",
			_DCACHE.IPIDR,
			(_DCACHE.VERR >> 4) & 0xf,
			_DCACHE.VERR & 0xf);
		g_log("ECC available:    %s\n", (_DCACHE.HWCFGR & DCACHE_HWCFGR_ECC) ? "yes" : "no");
		g_log("AHBM interface:   %d bits\n", DCACHE::GetAHBMWidth());
		g_log("Line width:       %d bytes\n", DCACHE::GetCacheLineWidth());
		g_log("Cache size:       %d kB\n", DCACHE::GetCacheSize());
		g_log("Associativity:    %d way\n", DCACHE::GetNumWays());
		g_log("Current status:   %s\n", DCACHE::IsEnabled() ? "enabled" : "disabled");
		g_log("Read hits:        %u\n", DCACHE::PerfGetReadHitCount());
		g_log("Read misses:      %u\n", DCACHE::PerfGetReadMissCount());
		g_log("Write hits:       %u\n", DCACHE::PerfGetWriteHitCount());
		g_log("Write misses:     %u\n", DCACHE::PerfGetWriteMissCount());
	#endif
}

void PrintFuseInfo()
{
	g_log("We are boot CPU, printing fuse information\n");
	LogIndenter li(g_log);

	//Turn on the fuse block since all of the MP2 config is in fuses
	RCCHelper::Enable(&_BSEC);

	if(BSEC::ReadFuse(BSEC_VIRGIN) == 0)
		g_log(Logger::ERROR, "OTP_HW_WORD0 is zero, expected nonzero value (access issue?)\n");

	const char* part = GetPartName(BSEC::ReadFuse(BSEC_RPN));
	uint32_t rev_id = BSEC::ReadFuse(BSEC_REV_ID) & 0x1f;
	auto pkg = GetPackage(BSEC::ReadFuse(BSEC_PKG) & 7);
	g_log("STM32%s stepping %d, %s\n", part, rev_id, pkg);

	//TODO: read DBGMCU ID code properly
	uint32_t dbgmcu_idc = *reinterpret_cast<volatile uint32_t*>(0x4a010000);
	g_log("Device ID: %04x rev %04x\n", dbgmcu_idc & 0xfff, dbgmcu_idc >> 16);

	g_log("oem_fsbla_monotonic_counter = %08x\n", BSEC::ReadFuse(BSEC_FSBLA_COUNT));
	if(BSEC::ReadFuse(BSEC_BOOTROM_CONFIG_7) & 0x10)
		g_log("FSBL-A is AARCH32\n");
	else
		g_log("FSBL-A is AARCH64\n");
	/*_BSEC.OTPCR = 18;
	if(_BSEC.FVR[18] & 0xf)
		g_log("Secure boot active (CLOSED_LOCKED)\n");
	else
		g_log("Secure boot disabled (CLOSED_UNLOCKED)\n");
	*/

	uint32_t U_ID[3] =
	{
		BSEC::ReadFuse(BSEC_ID_0),
		BSEC::ReadFuse(BSEC_ID_1),
		BSEC::ReadFuse(BSEC_ID_2)
	};
	uint16_t waferX = U_ID[0] >> 16;
	uint16_t waferY = U_ID[0] & 0xffff;
	uint8_t waferNum = U_ID[1] & 0xff;

	char waferLot[8] =
	{
		static_cast<char>((U_ID[1] >> 8) & 0xff),
		static_cast<char>((U_ID[1] >> 16) & 0xff),
		static_cast<char>((U_ID[1] >> 24) & 0xff),
		static_cast<char>((U_ID[2] >> 0) & 0xff),
		static_cast<char>((U_ID[2] >> 8) & 0xff),
		static_cast<char>((U_ID[2] >> 16) & 0xff),
		static_cast<char>((U_ID[2] >> 24) & 0xff),
		'\0'
	};
	g_log("Lot %s, wafer %d, die (%d, %d)\n", waferLot, waferNum, waferX, waferY);
}

#else
void __attribute__((weak)) BSP_DetectHardware()
{
	g_log("Identifying hardware\n");
	LogIndenter li(g_log);

	auto srev = GetStepping(DBGMCU.IDCODE >> 16);
	auto part = GetPartName(DBGMCU.IDCODE & 0xfff);

	#ifdef HAVE_PKG
		RCCHelper::EnableSyscfg();

		auto pkg = GetPackage(SYSCFG.PKGR & 0xf);
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
	#elif defined(STM32H750)
		g_log("1 MB total SRAM, 128 kB DTCM, 64 kB ITCM, 4 kB backup SRAM\n");
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
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Crack device IDs

#ifdef HAVE_PKG
const char* GetPackage(uint8_t pkg)
{
	#ifdef STM32H735
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
	#elif defined(STM32MP2)
		switch(pkg)
		{
			case 0:		return "Custom";
			case 1:		return "TFBGA361 (10x10 mm)";
			case 3:		return "TFBGA424";
			case 5:		return "TFBGA436";
			case 7:		return "TFBGA361 (16x16mm)";
			default:	return "reserved/unknown package";
		}
	#elif defined(STM32H750)
		switch(pkg)
		{
			case 0:		return "LQFP100";
			case 2:		return "TQFP144";
			case 5:		return "TQFP176/UFBGA176";
			case 8:		return "LQFP208/TFBGA240";
			default:	return "unknown package";
		}
	#endif
}
#endif

const char* GetStepping([[maybe_unused]] uint16_t rev)
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
	#elif defined(STM32H735)	//H735
		switch(rev)
		{
			case 0x1000:	return "A";
			case 0x1001:	return "Z";
			default:		return "(unknown)";
		}
	#elif defined(STM32H750)	//H750
		switch(rev)
		{
			case 0x1001:	return "Z";
			case 0x1003:	return "Y";
			case 0x2001:	return "X";
			case 0x2003:	return "V";
			default:		return "(unknown)";
		}
	#endif

	//if we get here, unrecognized
	return "(unknown)";
}

const char* GetPartName(
	#ifdef STM32MP2
		[[maybe_unused]] uint32_t device
	#else
		[[maybe_unused]] uint16_t device
	#endif
	)
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
	#elif defined(STM32H750)
		//0x450 is H742/743/753/750
		switch(device)
		{
			case 0x450:	return "H742/743/750/753";
			default:	return "(unknown)";
		}

	//735 in particular has text ID in L_ID
	#elif defined(STM32H735)
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

	#elif defined(STM32MP2)
		switch(device)
		{
			case 0x0000'2000:	return "MP257C";
			case 0x0008'2000:	return "MP255C";
			case 0x000b'300c:	return "MP253C";
			case 0x000b'306d:	return "MP251C";
			case 0x4000'2e00:	return "MP257A";
			case 0x4008'2e00:	return "MP255A";
			case 0x400b'3e0c:	return "MP253A";
			case 0x400b'3e6d:	return "MP251A";
			case 0x8000'2000:	return "MP257F";
			case 0x8008'2000:	return "MP255F";
			case 0x800b'300c:	return "MP253F";
			case 0x800b'306d:	return "MP251F";
			case 0xc000'2e00:	return "MP257D";
			case 0xc008'2e00:	return "MP255D";
			case 0xc00b'3e0c:	return "MP253D";
			case 0xc00b'3e6d:	return "MP251D";
			default:			return "(unknown)";
		}
	#endif

	//if we get here, unrecognized
	return "(unknown)";
}
