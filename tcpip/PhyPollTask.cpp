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

#include <core/platform.h>
#include "CommonTCPIP.h"
#include <fpga/Ethernet.h>
#include "PhyPollTask.h"

void PhyPollTask::PollPHYs()
{
	//Get the baseT link state
	uint16_t bctl = g_phyMdio->ReadRegister(REG_BASIC_CONTROL);
	uint16_t bstat = g_phyMdio->ReadRegister(REG_BASIC_STATUS);

	bool bup = (bstat & 4) == 4;
	if(bup && !g_basetLinkUp)
	{
		uint8_t speed = 0;
		 if( (bctl & 0x40) == 0x40)
			speed |= 2;
		if( (bctl & 0x2000) == 0x2000)
			speed |= 1;
		g_basetLinkSpeed = static_cast<linkspeed_t>(speed);

		g_log("Interface mgmt0: link is up at %s\n", g_linkSpeedNamesLong[g_basetLinkSpeed]);
		OnEthernetLinkStateChanged();
		g_ethProtocol->OnLinkUp();
	}
	else if(!bup && g_basetLinkUp)
	{
		g_log("Interface mgmt0: link is down\n");
		g_basetLinkSpeed = LINK_SPEED_10M;
		OnEthernetLinkStateChanged();
		g_ethProtocol->OnLinkDown();
	}
	g_basetLinkUp = bup;
}

void __attribute__((weak)) OnEthernetLinkStateChanged()
{
}
