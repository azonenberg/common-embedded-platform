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

//TODO: ifdefs to specify what type of fpga we have
#include <APB_DeviceInfo_7series.h>
extern volatile APB_DeviceInfo_7series FDEVINFO;

///@brief USERCODE of the FPGA (build timestamp)
uint32_t g_usercode = 0;

///@brief FPGA die serial number
uint8_t g_fpgaSerial[8] = {0};

/**
	@brief Initialize our FPGA

	Assumes we have a device info block called FDEVINFO somewhere, and that the USERCODE is set to the build date
 */
void InitFPGA()
{
	g_log("Initializing FPGA\n");
	LogIndenter li(g_log);

	//Verify reliable functionality by poking the scratchpad register (TODO: proper timing-control link training?)
	g_log("FPGA loopback test...\n");
	{
		LogIndenter li2(g_log);
		uint32_t tmp = 0xbaadc0de;
		uint32_t count = 1000;
		uint32_t errs = 0;
		for(uint32_t i=0; i<count; i++)
		//for(uint32_t i=0; true; i++)
		{
			FDEVINFO.scratch = tmp;
			uint32_t readback = FDEVINFO.scratch;
			if(readback != tmp)
			{
				//if(errs == 0)
					g_log(Logger::ERROR, "Iteration %u: wrote 0x%08x, read 0x%08x\n", i, tmp, readback);
				errs ++;
			}
			tmp ++;
		}
		g_log("%u iterations complete, %u errors\n", count, errs);
	}

	//Read the FPGA IDCODE and serial number
	while(FDEVINFO.status != 3)
	{}

	uint32_t idcode = FDEVINFO.idcode;
	memcpy(g_fpgaSerial, (const void*)FDEVINFO.serial, 8);

	//Print status
	switch(idcode & 0x0fffffff)
	{
		case 0x3647093:
			g_log("IDCODE: %08x (XC7K70T rev %d)\n", idcode, idcode >> 28);
			break;

		case 0x364c093:
			g_log("IDCODE: %08x (XC7K160T rev %d)\n", idcode, idcode >> 28);
			break;

		case 0x37c4093:
			g_log("IDCODE: %08x (XC7S25 rev %d)\n", idcode, idcode >> 28);
			break;

		case 0x37c7093:
			g_log("IDCODE: %08x (XC7S100 rev %d)\n", idcode, idcode >> 28);
			break;

		case 0x4a63093:
			g_log("IDCODE: %08x (XCKU3P rev %d)\n", idcode, idcode >> 28);
			break;

		default:
			g_log("IDCODE: %08x (unknown device, rev %d)\n", idcode, idcode >> 28);
			break;
	}
	g_log("Serial: %02x%02x%02x%02x%02x%02x%02x%02x\n",
		g_fpgaSerial[7], g_fpgaSerial[6], g_fpgaSerial[5], g_fpgaSerial[4],
		g_fpgaSerial[3], g_fpgaSerial[2], g_fpgaSerial[1], g_fpgaSerial[0]);

	//Read USERCODE
	g_usercode = FDEVINFO.usercode;
	g_log("Usercode: %08x\n", g_usercode);
	{
		LogIndenter li2(g_log);

		//Format per XAPP1232:
		//31:27 day
		//26:23 month
		//22:17 year
		//16:12 hr
		//11:6 min
		//5:0 sec
		int day = g_usercode >> 27;
		int mon = (g_usercode >> 23) & 0xf;
		int yr = 2000 + ((g_usercode >> 17) & 0x3f);
		int hr = (g_usercode >> 12) & 0x1f;
		int min = (g_usercode >> 6) & 0x3f;
		int sec = g_usercode & 0x3f;
		g_log("Bitstream timestamp: %04d-%02d-%02d %02d:%02d:%02d\n",
			yr, mon, day, hr, min, sec);
	}
}