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

#include "supervisor-common.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals

//Addresses on the management I2C bus
const uint8_t g_tempI2cAddress = 0x90;
const uint8_t g_ibcI2cAddress = 0x42;

//The ADC (can't be initialized before InitClocks() so can't be a global object)
ADC* g_adc = nullptr;

//IBC version strings
char g_ibcSwVersion[20] = {0};
char g_ibcHwVersion[20] = {0};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hardware initialization

void Super_InitI2C()
{
	g_log("Initializing IBC I2C interface\n");

	//Initialize the I2C then wait a bit longer
	//(i2c pin states prior to init are unknown)
	g_logTimer.Sleep(100);
	g_i2c.Reset();
	g_logTimer.Sleep(100);

	//Set temperature sensor to max resolution
	//(if it doesn't respond, the i2c is derped out so reset and try again)
	for(int i=0; i<5; i++)
	{
		uint8_t cmd[3] = {0x01, 0x60, 0x00};
		if(g_i2c.BlockingWrite(g_tempI2cAddress, cmd, sizeof(cmd)))
			break;

		g_log(
			Logger::WARNING,
			"Failed to initialize I2C temp sensor at 0x%02x, resetting and trying again\n",
			g_tempI2cAddress);

		g_i2c.Reset();
		g_logTimer.Sleep(100);
	}
}

void Super_InitIBC()
{
	g_log("Connecting to IBC\n");
	LogIndenter li(g_log);

	//Wait a while to make sure the IBC is booted before we come up
	//(both us and the IBC come up off 3V3_SB as soon as it's up, with no sequencing)
	g_logTimer.Sleep(2500);

	g_i2c.BlockingWrite8(g_ibcI2cAddress, IBC_REG_VERSION);
	g_i2c.BlockingRead(g_ibcI2cAddress, (uint8_t*)g_ibcSwVersion, sizeof(g_ibcSwVersion));
	g_log("IBC firmware version %s\n", g_ibcSwVersion);

	g_i2c.BlockingWrite8(g_ibcI2cAddress, IBC_REG_HW_VERSION);
	g_i2c.BlockingRead(g_ibcI2cAddress, (uint8_t*)g_ibcHwVersion, sizeof(g_ibcHwVersion));
	g_log("IBC hardware version %s\n", g_ibcHwVersion);
}
