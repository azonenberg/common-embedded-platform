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

#include <core/platform.h>
#include <bootloader/bootloader-common.h>
#include <algorithm>

void App_Init();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// KVS settings

const char* g_imageVersionKey = "firmware.imageVersion";
const char* g_imageCRCKey = "firmware.crc";

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization

bool IsBootloader()
{
	return true;
}

void App_Init()
{
	g_log("Antikernel Labs bootloader (%s %s)\n", __DATE__, __TIME__);

	RCCHelper::Enable(&_CRC);

	//We need to enable the RTC on STM32L4 and H7 since backup registers are part of the RTC block
	#if defined(STM32L4) || defined(STM32H7)
		RCCHelper::Enable(&_RTC);
	#endif

	Bootloader_Init();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Boot flow

bool ValidateAppPartition(const uint32_t* appVector)
{
	g_log("Checking application partition at 0x%08x\n",
		reinterpret_cast<uint32_t>(appVector));
	LogIndenter li(g_log);

	//Vector table is blank? No app present
	if(appVector[0] == 0xffffffff)
	{
		g_log(Logger::ERROR, "Application partition appears to be blank\n");
		return false;
	}

	//See if we have a saved CRC in flash
	uint32_t expectedCRC = 0;
	const char* firmwareVer = nullptr;
	bool updatedViaJtag = IsAppUpdated(appVector, firmwareVer);
	if(!updatedViaJtag)
	{
		auto hlog = g_kvs->FindObject(g_imageCRCKey);
		if(!hlog)
		{
			g_log(Logger::WARNING, "Image version found in KVS, but not a CRC. Can't verify integrity\n");
			updatedViaJtag = true;
		}

		else
		{
			expectedCRC = *reinterpret_cast<uint32_t*>(g_kvs->MapObject(hlog));
			g_log("Expected image CRC:           %08x\n", expectedCRC);
		}
	}

	//CRC the entire application partition (including blank space)
	//Disable faults during the CRC so corrupted bit cells (double ECC failures) don't crash the bootoloader
	#ifdef HAVE_FLASH_ECC
		Flash::ClearECCFaults();
		auto sr = SCB_DisableDataFaults();
	#endif
	auto start = g_logTimer.GetCount();
	auto crc = CRC::Checksum((const uint8_t*)appVector, g_appImageSize);
	#ifdef HAVE_FLASH_ECC
		bool fail = Flash::CheckForECCFaults();
		uint32_t addr = Flash::GetFaultAddress();
		Flash::ClearECCFaults();
		SCB_EnableDataFaults(sr);
		if(fail)
		{
			g_log(Logger::ERROR, "Uncorrectable ECC error while checksumming image (at %08x)\n", addr);
			return false;
		}
	#endif
	auto dt = g_logTimer.GetCount() - start;
	g_log("CRC of application partition: %08x (took %d.%d ms)\n", crc, dt/10, dt%10);

	//If we detected a JTAG update, we need to update the saved version and checksum info
	if(updatedViaJtag)
	{
		g_log("New image present (JTAG flash?) but no corresponding saved CRC, updating CRC and version\n");

		if(!g_kvs->StoreObject(g_imageVersionKey, (const uint8_t*)firmwareVer, strlen(firmwareVer)))
			g_log(Logger::ERROR, "KVS write error\n");
		if(!g_kvs->StoreObject(g_imageCRCKey, (const uint8_t*)&crc, sizeof(crc)))
			g_log(Logger::ERROR, "KVS write error\n");
		return true;
	}

	//We are booting the same image we have in flash. Need to check integrity
	else if(crc == expectedCRC)
	{
		g_log("CRC verification passed\n");
		return true;
	}

	else
	{
		g_log(Logger::ERROR, "CRC mismatch, application partition flash corruption?\n");
		return false;
	}
}

bool IsAppUpdated(const uint32_t* appVector, const char*& firmwareVer)
{
	//Image is present, see if we have a good version string
	firmwareVer = reinterpret_cast<const char*>(appVector) + g_appVersionOffset;
	bool validVersion = false;
	for(size_t i=0; i<32; i++)
	{
		if(firmwareVer[i] == '\0')
		{
			validVersion = true;
			break;
		}
	}
	if(!validVersion)
	{
		g_log(Logger::ERROR, "No version string found in application partition!\n");
		g_log("Expected <32 byte null terminated string at 0x%08x\n",
			reinterpret_cast<uint32_t>(firmwareVer));

		return false;
	}
	g_log("Found firmware version:       %s\n", firmwareVer);

	//See if we're booting a previously booted image
	auto hlog = g_kvs->FindObject(g_imageVersionKey);
	if(hlog)
	{
		char knownVersion[33] = {0};
		strncpy(knownVersion, (const char*)g_kvs->MapObject(hlog), std::min(hlog->m_len, (uint32_t)32));
		g_log("Previous image version:       %s\n", knownVersion);

		//Is this the image we are now booting?
		if(0 != strcmp(knownVersion, firmwareVer))
			return true;
	}

	//Valid image but nothing in KVS, we must have just jtagged the first firmware
	else
	{
		g_log("No previous image version information in KVS\n");
		return true;
	}

	//If we get here, we're using the same firmware
	return false;
}

const char* GetImageVersion(const uint32_t* appVector)
{
	auto firmwareVer = reinterpret_cast<const char*>(appVector) + g_appVersionOffset;
	for(size_t i=0; i<32; i++)
	{
		if(firmwareVer[i] == '\0')
			return firmwareVer;
	}

	//no null terminator
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Flash helpers

void EraseFlash(uint32_t* appVector)
{
	//TODO: it's much more efficient to only erase the stuff we are overwriting, instead of the whole chip
	g_log("Erasing application flash partition\n");
	LogIndenter li(g_log);

	uint8_t* appStart				= reinterpret_cast<uint8_t*>(appVector);

	#if defined(STM32L431)
		const uint32_t eraseBlockSize	= 2 * 1024;
	#elif defined(STM32L031)
		const uint32_t eraseBlockSize	= 128;
	#elif defined(STM32H735)
		const uint32_t eraseBlockSize	= 128 * 1024;
	#else
		#error Dont know erase block size for this chip!
	#endif

	const uint32_t nblocks 			= g_appImageSize / eraseBlockSize;

	auto start = g_logTimer.GetCount();
	for(uint32_t i=0; i<nblocks; i++)
	{
		if( (i % 10) == 0)
		{
			auto dt = g_logTimer.GetCount() - start;
			g_log("Block %u / %u (elapsed %d.%d ms)\n", i, nblocks, dt / 10, dt % 10);
		}

		Flash::BlockErase(appStart + i*eraseBlockSize);

		//discard any commands that showed up while we were busy
		Bootloader_ClearRxBuffer();
	}

	auto dt = g_logTimer.GetCount() - start;
	g_log("Done (in %d.%d ms)\n", dt / 10, dt % 10);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Boot the application

void __attribute__((noreturn)) BootApplication(const uint32_t* appVector)
{
	//Debug delay in case we bork something
	#ifdef DEBUG_BOOT_DELAY
		g_logTimer.Sleep(10000);
	#endif

	//Print our final log message and flush the transmit FIFO before transferring control to the application
	g_log("Booting application...\n\n");
	Bootloader_FinalCleanup();

	g_bbram->m_state = STATE_APP;
	DoBootApplication(appVector);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Top level main loop for the bootloader

void Bootloader_MainLoop()
{
	//Check bbram state
	bool goStraightToDFU = false;
	bool crashed = false;
	g_log("Checking reason for last reset...\n");
	{
		LogIndenter li(g_log);
		switch(g_bbram->m_state)
		{
			case STATE_POR:
				g_log("Power cycle\n");
				break;

			case STATE_APP:
				g_log("Application was running, probably requested warm reboot\n");
				break;

			case STATE_DFU:
				g_log("Application requested DFU entry\n");
				goStraightToDFU = true;
				break;

			case STATE_CRASH:
				crashed = true;
				switch(g_bbram->m_crashReason)
				{
					case CRASH_UNUSED_ISR:
						g_log(Logger::ERROR, "Unused ISR called\n");
						break;

					case CRASH_NMI:
						g_log(Logger::ERROR, "NMI\n");
						break;

					case CRASH_HARD_FAULT:
						g_log(Logger::ERROR, "Hard fault\n");
						break;

					case CRASH_BUS_FAULT:
						g_log(Logger::ERROR, "Bus fault\n");
						break;

					case CRASH_USAGE_FAULT:
						g_log(Logger::ERROR, "Usage fault\n");
						break;

					case CRASH_MMU_FAULT:
						g_log(Logger::ERROR, "MMU fault\n");
						break;

					default:
						g_log(Logger::ERROR, "Unknown crash code\n");
						break;
				}
				break;

			default:
				g_log("Last reset from unknown cause\n");
				break;
		}
	}

	//Skip all other processing if a DFU was requested
	if(goStraightToDFU)
		Bootloader_FirmwareUpdateFlow();

	//Application crashed? Don't try to run the crashy app again to avoid bootlooping
	//TODO: give it a couple of tries first?
	else if(crashed)
	{
		const char* firmwareVer = nullptr;
		if(IsAppUpdated(g_appVector, firmwareVer))
		{
			g_log("Application was updated since last flash, attempting to boot new image\n");
			if(ValidateAppPartition(g_appVector))
				BootApplication(g_appVector);
			else
				Bootloader_FirmwareUpdateFlow();
		}

		else
		{
			g_log("Still running same crashy binary, going to DFU flow\n");
			Bootloader_FirmwareUpdateFlow();
		}
	}

	//Corrupted image? Go to DFU, it's our only hope
	else if(!ValidateAppPartition(g_appVector))
		Bootloader_FirmwareUpdateFlow();

	//If we get to this point, the image is valid, boot it
	else
		BootApplication(g_appVector);
}
